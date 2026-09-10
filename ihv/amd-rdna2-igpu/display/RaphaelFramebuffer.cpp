#include "RaphaelFramebuffer.h"
#include "RaphaelConnectorNub.h"
#include "../match/RaphaelController.h"

#include <IOKit/IODeviceMemory.h>
#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <pexpert/i386/boot.h>
#include <pexpert/pexpert.h>

#define super IOFBLinearShell
OSDefineMetaClassAndStructors(RaphaelFramebuffer, IOFBLinearShell);

static const uint32_t kRaphaelGopMinBytes = 640 * 480 * 4;
static const uint32_t kRaphaelGopMaxBytes = 512 * 1024 * 1024;

static bool gopAddressIsPhysical(uint64_t addr)
{
	return addr >= 0x1000000ULL && (addr & 0xffff000000000000ULL) == 0;
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

bool RaphaelFramebuffer::captureGopAperture(void)
{
	uint64_t phys = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t rowBytes = 0;
	uint32_t depth = 32;

	boot_args *args = reinterpret_cast<boot_args *>(PE_state.bootArgs);
	if (args) {
		width = args->Video.v_width;
		height = args->Video.v_height;
		rowBytes = args->Video.v_rowBytes;
		depth = gopDepthBits(args->Video.v_depth);
		phys = args->Video.v_baseAddr;
		if (phys == 0)
			phys = args->VideoV1.v_baseAddr;
	}

	if (width == 0 || height == 0 || phys == 0) {
		PE_Video console = {};
		if (PE_current_console(&console) == 0) {
			if (width == 0)
				width = (uint32_t)console.v_width;
			if (height == 0)
				height = (uint32_t)console.v_height;
			if (rowBytes == 0)
				rowBytes = (uint32_t)console.v_rowBytes;
			if (phys == 0)
				phys = (uint64_t)console.v_baseAddr;
			if (console.v_depth)
				depth = gopDepthBits((uint32_t)console.v_depth);
		}
	}

	if (width == 0 || height == 0) {
		width = (uint32_t)PE_state.video.v_width;
		height = (uint32_t)PE_state.video.v_height;
		if (rowBytes == 0)
			rowBytes = (uint32_t)PE_state.video.v_rowBytes;
		if (phys == 0)
			phys = (uint64_t)PE_state.video.v_baseAddr;
	}

	UInt32 arg = 0;
	if (PE_parse_boot_argn("raphael_width", &arg, sizeof(arg)) && arg >= 640)
		width = arg;
	arg = 0;
	if (PE_parse_boot_argn("raphael_height", &arg, sizeof(arg)) && arg >= 480)
		height = arg;

	if (rowBytes < width)
		rowBytes = width * 4;
	if (depth != 32) {
		IOLog("RaphaelFramebuffer: GOP depth %u not 32bpp, leaving IONDRV\n", depth);
		return false;
	}
	if (width < 640 || height < 480 || !gopAddressIsPhysical(phys)) {
		IOLog("RaphaelFramebuffer: no physical GOP aperture (phys=0x%llx %ux%u), leaving IONDRV\n",
		      phys, width, height);
		return false;
	}

	IOByteCount bytes = (IOByteCount)rowBytes * (IOByteCount)height;
	if (PE_state.video.v_length > 0 && (IOByteCount)PE_state.video.v_length < bytes)
		bytes = (IOByteCount)PE_state.video.v_length;
	if (bytes < kRaphaelGopMinBytes || bytes > kRaphaelGopMaxBytes) {
		IOLog("RaphaelFramebuffer: GOP size %llu rejected, leaving IONDRV\n",
		      (unsigned long long)bytes);
		return false;
	}

	/*
	 * Describe the firmware GOP range. Do not setMemoryEnable or
	 * mapDeviceMemory — that hung v0.1.2 while IONDRV still owned the head.
	 */
	fGopMemory = IODeviceMemory::withRange((IOPhysicalAddress)phys, bytes);
	if (!fGopMemory) {
		IOLog("RaphaelFramebuffer: IODeviceMemory::withRange failed, leaving IONDRV\n");
		return false;
	}

	fWidth = width;
	fHeight = height;
	fRowBytes = rowBytes;
	IOLog("RaphaelFramebuffer: GOP phys=0x%llx %ux%u pitch=%u bytes=%llu\n", phys, width, height,
	      rowBytes, (unsigned long long)bytes);
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
		const AtomParseResult &cons = fController->connectors();
		if (cons.connectorCount > 0) {
			fSpec.enumId = cons.connectors[0].enumId;
			fSpec.deviceTag = cons.connectors[0].deviceTag;
			fSpec.objectId = cons.connectors[0].objectId;
			if (cons.connectors[0].kind != kRaphaelConnectorUnknown)
				fSpec.kind = cons.connectors[0].kind;
		}
	}
	return true;
}

bool RaphaelFramebuffer::start(IOService *provider)
{
	if (!attachController(provider)) {
		IOLog("RaphaelFramebuffer: provider is not Raphael PCI/nub\n");
		return false;
	}
	if (fBootHead && !captureGopAperture()) {
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
		*value = kIOConnectionBuiltIn;
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

bool RaphaelFramebuffer::hasDDCConnect(IOIndex connectIndex)
{
	return connectIndex == 0 && fBootHead;
}
