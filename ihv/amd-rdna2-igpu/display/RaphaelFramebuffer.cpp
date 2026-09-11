#include "RaphaelFramebuffer.h"
#include "RaphaelConnectorNub.h"
#include "../match/RaphaelController.h"

#include <IOKit/IODeviceMemory.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/graphics/IOGraphicsTypes.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <libkern/libkern.h>
#include <pexpert/pexpert.h>

#define super IOFBLinearShell
OSDefineMetaClassAndStructors(RaphaelFramebuffer, IOFBLinearShell);

static const uint32_t kRaphaelGopMinBytes = 640 * 480 * 4;
static const uint32_t kRaphaelGopMaxBytes = 512 * 1024 * 1024;

static bool gopAddressIsPhysical(uint64_t addr)
{
	return addr >= 0x1000000ULL && (addr & 0xffff000000000000ULL) == 0;
}

static bool gopPhysIsUsable(uint64_t addr)
{
	if (!gopAddressIsPhysical(addr))
		return false;
	/* Tahoe getConsoleInfo returned 0x10000000001 — not a page, not a BAR. */
	if ((addr & 0xfffULL) != 0)
		return false;
	return true;
}

/*
 * Firmware GOP lives in a PCI aperture already described on the nub.
 * Do not mapDeviceMemory / setMemoryEnable (v0.1.2 hang).
 */
static IODeviceMemory *gopMemoryFromPci(IOPCIDevice *pci, uint64_t preferPhys, IOByteCount bytes)
{
	if (!pci || bytes == 0)
		return nullptr;
	const UInt32 n = pci->getDeviceMemoryCount();
	for (UInt32 i = 0; i < n; i++) {
		IODeviceMemory *bar = pci->getDeviceMemoryWithIndex(i);
		if (!bar || bar->getLength() < bytes)
			continue;
		IOByteCount segLen = 0;
		const addr64_t barPhys = bar->getPhysicalSegment(0, &segLen, kIOMemoryMapperNone);
		IOByteCount origin = 0;
		if (gopPhysIsUsable(preferPhys) && barPhys != 0 && preferPhys >= barPhys &&
		    (preferPhys - barPhys) + bytes <= bar->getLength())
			origin = (IOByteCount)(preferPhys - barPhys);
		IODeviceMemory *sub = IODeviceMemory::withSubRange(bar, origin, bytes);
		if (sub) {
			IOLog("RaphaelFramebuffer: GOP from PCI BAR%u phys=0x%llx origin=%llu bytes=%llu\n",
			      i, (unsigned long long)(barPhys + origin), (unsigned long long)origin,
			      (unsigned long long)bytes);
			return sub;
		}
	}
	return nullptr;
}

static uint32_t gopDepthBits(uint32_t depth)
{
	if (depth == 0)
		return 32;
	if (depth <= 8)
		return depth * 8;
	return depth;
}

bool RaphaelFramebuffer::init(OSDictionary *dictionary)
{
	if (!super::init(dictionary))
		return false;
	fController = nullptr;
	fGopMemory = nullptr;
	fConnectorIndex = 0;
	fBootHead = false;
	fSwapRB = false;
	fWidth = kRaphaelDefaultWidth;
	fHeight = kRaphaelDefaultHeight;
	fRowBytes = 0;
	bzero(&fSpec, sizeof(fSpec));
	return true;
}

IOService *RaphaelFramebuffer::probe(IOService *provider, SInt32 *score)
{
	int dummy = 1;
	if (PE_parse_boot_argn("raphael_fb", &dummy, sizeof(dummy)) && dummy == 0)
		return nullptr;
	return super::probe(provider, score);
}

void RaphaelFramebuffer::releaseGopMemory(void)
{
	if (fGopMemory) {
		fGopMemory->release();
		fGopMemory = nullptr;
	}
}

void RaphaelFramebuffer::releaseController(void)
{
	if (fController) {
		fController->release();
		fController = nullptr;
	}
}

bool RaphaelFramebuffer::captureGopAperture(IOService *provider)
{
	uint64_t phys = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t rowBytes = 0;
	uint32_t depth = 32;
	IOByteCount vLength = 0;

	/*
	 * AuxKC cannot bind pexpert internals (_PE_current_console, _PE_state).
	 * IOPlatformExpert::getConsoleInfo is an IOKit virtual on the platform
	 * expert already in the System collection.
	 */
	PE_Video console = {};
	IOPlatformExpert *pe = IOService::getPlatform();
	if (pe && pe->getConsoleInfo(&console) == kIOReturnSuccess) {
		width = (uint32_t)console.v_width;
		height = (uint32_t)console.v_height;
		rowBytes = (uint32_t)console.v_rowBytes;
		phys = (uint64_t)console.v_baseAddr + (uint64_t)console.v_offset;
		if (console.v_depth)
			depth = gopDepthBits((uint32_t)console.v_depth);
		if (console.v_length)
			vLength = (IOByteCount)console.v_length;
		IOLog("RaphaelFramebuffer: console base=0x%lx off=0x%lx len=%lu %lux%lux%lu pitch=%lu fmt=%s\n",
		      console.v_baseAddr, console.v_offset, console.v_length, console.v_width,
		      console.v_height, console.v_depth, console.v_rowBytes,
		      console.v_pixelFormat[0] ? console.v_pixelFormat : "-");
		if (console.v_pixelFormat[0] &&
		    (strncmp(console.v_pixelFormat, "RGBA", 4) == 0 ||
		     strncmp(console.v_pixelFormat, "XR24", 4) == 0))
			fSwapRB = true;
	}

	UInt32 arg = 0;
	if (PE_parse_boot_argn("raphael_width", &arg, sizeof(arg)) && arg >= 640)
		width = arg;
	arg = 0;
	if (PE_parse_boot_argn("raphael_height", &arg, sizeof(arg)) && arg >= 480)
		height = arg;
	arg = 0;
	if (PE_parse_boot_argn("raphael_swap_rb", &arg, sizeof(arg)))
		fSwapRB = arg != 0;

	if (rowBytes < width)
		rowBytes = width * 4;
	if (depth != 32) {
		IOLog("RaphaelFramebuffer: GOP depth %u not 32bpp, leaving IONDRV\n", depth);
		return false;
	}
	if (width < 640 || height < 480) {
		IOLog("RaphaelFramebuffer: GOP size %ux%u rejected, leaving IONDRV\n", width, height);
		return false;
	}

	IOByteCount bytes = (IOByteCount)rowBytes * (IOByteCount)height;
	if (vLength > 0 && vLength < bytes)
		bytes = vLength;
	if (bytes < kRaphaelGopMinBytes || bytes > kRaphaelGopMaxBytes) {
		IOLog("RaphaelFramebuffer: GOP size %llu rejected, leaving IONDRV\n",
		      (unsigned long long)bytes);
		return false;
	}

	IOPCIDevice *pci = nullptr;
	if (fController)
		pci = fController->pciDevice();
	if (!pci)
		pci = OSDynamicCast(IOPCIDevice, provider);

	/*
	 * Prefer a PCI BAR range already on the nub. getConsoleInfo v_baseAddr
	 * is not a usable physical (lab: 0x10000000001). Do not setMemoryEnable
	 * or mapDeviceMemory — that hung v0.1.2.
	 */
	fGopMemory = gopMemoryFromPci(pci, phys, bytes);
	if (!fGopMemory && gopPhysIsUsable(phys)) {
		fGopMemory = IODeviceMemory::withRange((IOPhysicalAddress)phys, bytes);
		if (fGopMemory)
			IOLog("RaphaelFramebuffer: GOP withRange phys=0x%llx bytes=%llu\n", phys,
			      (unsigned long long)bytes);
	}
	if (!fGopMemory) {
		IOLog("RaphaelFramebuffer: no GOP/BAR aperture (console phys=0x%llx), leaving IONDRV\n",
		      phys);
		return false;
	}

	fWidth = width;
	fHeight = height;
	fRowBytes = rowBytes;
	IOLog("RaphaelFramebuffer: GOP wrap %ux%u pitch=%u bytes=%llu swapRB=%d\n", width, height,
	      rowBytes, (unsigned long long)bytes, fSwapRB ? 1 : 0);
	return true;
}

void RaphaelFramebuffer::applyGopMode(void)
{
	const IOFBLinearMode mode = { 0x00000547, fWidth, fHeight, kRaphaelDefaultRefreshHz,
				      fRowBytes };
	setModeTable(&mode, 1, 0x00000547);
	setProperty("IOFBMemorySize", (UInt32)(fRowBytes * fHeight), 32);
}

bool RaphaelFramebuffer::attachController(IOService *provider)
{
	RaphaelConnectorNub *nub = OSDynamicCast(RaphaelConnectorNub, provider);
	if (nub) {
		fController = nub->controller();
		if (fController)
			fController->retain();
		fConnectorIndex = nub->connectorIndex();
		fSpec = nub->spec();
		fBootHead = false;
		return fController != nullptr;
	}

	IOPCIDevice *pci = OSDynamicCast(IOPCIDevice, provider);
	if (!pci)
		return false;

	fSpec.kind = kRaphaelConnectorGop;
	fSpec.preferOnline = true;
	fBootHead = true;

	RaphaelController *found = RaphaelController::withProvider(provider);
	if (found) {
		found->retain();
		fController = found;
	} else {
		OSDictionary *matching = IOService::serviceMatching("RaphaelController");
		IOService *svc = waitForMatchingService(matching, 2 * 1000000000ULL);
		if (matching)
			matching->release();
		fController = OSDynamicCast(RaphaelController, svc);
		if (!fController && svc)
			svc->release();
	}

	if (fController && fController->pciDevice() && fController->pciDevice() != pci) {
		releaseController();
		return false;
	}
	if (fController) {
		fSpec = fController->bootConnectorSpec();
		fConnectorIndex = fController->bootConnectorIndex();
	}
	return true;
}

bool RaphaelFramebuffer::start(IOService *provider)
{
	if (!attachController(provider)) {
		IOLog("RaphaelFramebuffer: provider is not Raphael PCI/nub\n");
		return false;
	}
	if (fBootHead && !captureGopAperture(provider)) {
		releaseController();
		return false;
	}
	if (!fBootHead && fController && fController->apertureMemory()) {
		IODeviceMemory *bar0 = fController->apertureMemory();
		bar0->retain();
		fGopMemory = bar0;
		fWidth = kRaphaelDefaultWidth;
		fHeight = kRaphaelDefaultHeight;
		fRowBytes = fWidth * 4;
	}
	if (!fGopMemory) {
		IOLog("RaphaelFramebuffer: no GOP/BAR aperture, leaving IONDRV\n");
		releaseController();
		return false;
	}

	applyGopMode();
	if (!super::start(provider)) {
		releaseGopMemory();
		releaseController();
		return false;
	}

	setName("RaphaelFramebuffer");
	setProperty("connector-kind", RaphaelConnectorKindName(fSpec.kind));
	setProperty("RaphaelPhase", "R2-gop-wrap");
	IOLog("RaphaelFramebuffer: %s index %u boot=%d %ux%u\n",
	      RaphaelConnectorKindName(fSpec.kind), fConnectorIndex, fBootHead ? 1 : 0, fWidth,
	      fHeight);
	/*
	 * BAR5 probe/dump after GOP wrap so a hang is this map, not FB attach.
	 * Default (raphael_dcn_probe / raphael_dcn_dump unset) does not map DCN.
	 */
	if (fBootHead && fController) {
		fController->probeDcnBar5IfRequested();
		fController->runHwModesetIfRequested();
	}
	return true;
}

void RaphaelFramebuffer::stop(IOService *provider)
{
	releaseGopMemory();
	releaseController();
	super::stop(provider);
}

bool RaphaelFramebuffer::isConsoleDevice(void)
{
	return fBootHead;
}

IOReturn RaphaelFramebuffer::enableController(void)
{
	IODeviceMemory *mem = copyApertureMemory();
	if (!mem)
		return kIOReturnNoMemory;
	mem->release();
	return super::enableController();
}

IOReturn RaphaelFramebuffer::getPixelInformation(IODisplayModeID displayMode, IOIndex depth,
						 IOPixelAperture aperture,
						 IOPixelInformation *pixelInfo)
{
	const IOReturn kr = super::getPixelInformation(displayMode, depth, aperture, pixelInfo);
	if (kr != kIOReturnSuccess || !pixelInfo || !fSwapRB)
		return kr;
	pixelInfo->componentMasks[0] = 0x000000FF;
	pixelInfo->componentMasks[1] = 0x0000FF00;
	pixelInfo->componentMasks[2] = 0x00FF0000;
	return kIOReturnSuccess;
}

IODeviceMemory *RaphaelFramebuffer::copyApertureMemory(void)
{
	if (!fGopMemory)
		return nullptr;
	const IOFBLinearMode *mode = modeById(fCurrentMode);
	IOByteCount need = fGopMemory->getLength();
	if (mode) {
		const UInt32 pitch = mode->bytesPerRow ? mode->bytesPerRow : mode->width * 4;
		const IOByteCount modeBytes = (IOByteCount)pitch * mode->height;
		if (modeBytes > 0 && modeBytes <= fGopMemory->getLength())
			need = modeBytes;
	}
	IODeviceMemory *sub = IODeviceMemory::withSubRange(fGopMemory, 0, need);
	if (sub)
		return sub;
	fGopMemory->retain();
	return fGopMemory;
}

IOReturn RaphaelFramebuffer::getAttributeForConnection(IOIndex connectIndex, IOSelect attribute,
						       uintptr_t *value)
{
	if (connectIndex != 0 || !value)
		return kIOReturnBadArgument;
	const bool online = fBootHead || (fSpec.preferOnline && fController &&
					  fController->forceAllConnectors() &&
					  fSpec.kind != kRaphaelConnectorUsbC);
	switch (attribute) {
	case kConnectionEnable:
		*value = online ? 1 : 0;
		return kIOReturnSuccess;
	case kConnectionCheckEnable:
		*value = online ? 1 : 0;
		return kIOReturnSuccess;
	case kConnectionFlags:
		/*
		 * Kernel.framework IOGraphicsTypes.h: kIOConnectionBuiltIn
		 * (0x00000800) is the documented “internal” flag. External
		 * HDMI/DP must not set it (System Settings showed Internal).
		 * eDP would be BuiltIn; this VFCT SKU has no eDP.
		 */
		*value = (fSpec.kind == kRaphaelConnectorEdp) ? kIOConnectionBuiltIn : 0;
		return kIOReturnSuccess;
	default:
		return super::getAttributeForConnection(connectIndex, attribute, value);
	}
}

IOReturn RaphaelFramebuffer::setAttributeForConnection(IOIndex connectIndex, IOSelect attribute,
						       uintptr_t value)
{
	if (connectIndex != 0)
		return kIOReturnBadArgument;
	return super::setAttributeForConnection(connectIndex, attribute, value);
}

IOReturn RaphaelFramebuffer::connectFlags(IOIndex connectIndex, IODisplayModeID,
					  IOOptionBits *flags)
{
	if (connectIndex != 0 || !flags)
		return kIOReturnBadArgument;
	*flags = kDisplayModeValidFlag | kDisplayModeSafeFlag;
	return kIOReturnSuccess;
}

IOReturn RaphaelFramebuffer::setDisplayMode(IODisplayModeID displayMode, IOIndex depth)
{
	const IOFBLinearMode *mode = modeById(displayMode);
	if (depth != 0 || !mode)
		return kIOReturnUnsupported;

	const bool gopMode = mode->width == fWidth && mode->height == fHeight;
	const bool wantHw = fController && fController->dcnModesetRequested();

	if (!gopMode) {
		IOLog("RaphaelFramebuffer: setDisplayMode %ux%u unsupported (GOP wrap is %ux%u; "
		      "first HW path is live 3840x2160 only)\n",
		      mode->width, mode->height, fWidth, fHeight);
		return kIOReturnUnsupported;
	}

	if (wantHw && fController) {
		if (fController->dcnHandshakeOk() || fController->runHwModesetIfRequested())
			IOLog("RaphaelFramebuffer: setDisplayMode hardware %ux%u (OTG live DP pipe)\n",
			      mode->width, mode->height);
		else
			IOLog("RaphaelFramebuffer: setDisplayMode software %ux%u (DMUB/modeset "
			      "failed; GOP wrap)\n",
			      mode->width, mode->height);
	} else {
		IOLog("RaphaelFramebuffer: setDisplayMode software %ux%u (GOP wrap, no DCN write)\n",
		      mode->width, mode->height);
	}
	return super::setDisplayMode(displayMode, depth);
}

bool RaphaelFramebuffer::hasDDCConnect(IOIndex connectIndex)
{
	return connectIndex == 0 && fBootHead;
}
