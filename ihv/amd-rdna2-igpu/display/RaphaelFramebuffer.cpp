#include "RaphaelFramebuffer.h"
#include "RaphaelConnectorNub.h"
#include "../match/RaphaelController.h"

#include <IOKit/IODeviceMemory.h>
#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <pexpert/pexpert.h>

#define super IOFBLinearShell
OSDefineMetaClassAndStructors(RaphaelFramebuffer, IOFBLinearShell);

void RaphaelFramebuffer::releaseController(void)
{
	if (fBootHead && fController) {
		fController->release();
	}
	fController = nullptr;
}

void RaphaelFramebuffer::applyGopMode(void)
{
	fWidth = kRaphaelDefaultWidth;
	fHeight = kRaphaelDefaultHeight;
	PE_parse_boot_argn("raphael_width", &fWidth, sizeof(fWidth));
	PE_parse_boot_argn("raphael_height", &fHeight, sizeof(fHeight));
	if (fWidth < 640)
		fWidth = 640;
	if (fHeight < 480)
		fHeight = 480;
	const IOFBLinearMode mode = { 0x00000547, fWidth, fHeight, kRaphaelDefaultRefreshHz };
	setModeTable(&mode, 1, 0x00000547);
	setProperty("IOFBMemorySize", (UInt32)(fWidth * fHeight * 4), 32);
}

bool RaphaelFramebuffer::start(IOService *provider)
{
	fController = nullptr;
	fConnectorIndex = 0;
	fBootHead = false;
	fWidth = kRaphaelDefaultWidth;
	fHeight = kRaphaelDefaultHeight;
	bzero(&fSpec, sizeof(fSpec));

	RaphaelConnectorNub *nub = OSDynamicCast(RaphaelConnectorNub, provider);
	if (nub) {
		fController = nub->controller();
		fConnectorIndex = nub->connectorIndex();
		fSpec = nub->spec();
		fBootHead = false;
	} else {
		IOPCIDevice *pci = OSDynamicCast(IOPCIDevice, provider);
		if (!pci)
			return false;
		OSDictionary *matching = IOService::serviceMatching("RaphaelController");
		IOService *found = waitForMatchingService(matching, 5 * 1000000000ULL);
		if (matching)
			matching->release();
		fController = OSDynamicCast(RaphaelController, found);
		if (!fController) {
			if (found)
				found->release();
			return false;
		}
		if (fController->pciDevice() != pci) {
			fController->release();
			fController = nullptr;
			return false;
		}
		fSpec.kind = kRaphaelConnectorGop;
		fSpec.preferOnline = true;
		const AtomParseResult &cons = fController->connectors();
		if (cons.connectorCount > 0) {
			fSpec.enumId = cons.connectors[0].enumId;
			fSpec.deviceTag = cons.connectors[0].deviceTag;
			fSpec.objectId = cons.connectors[0].objectId;
		}
		fBootHead = true;
	}

	if (!fController || !super::start(provider)) {
		releaseController();
		return false;
	}

	applyGopMode();
	setProperty("connector-kind", RaphaelConnectorKindName(fSpec.kind));
	setProperty("model", kRaphaelModelName);
	setProperty("RaphaelPhase", "R2-gop-wrap");

	IOLog("RaphaelFramebuffer: %s index %u boot=%d %ux%u\n",
	      RaphaelConnectorKindName(fSpec.kind), fConnectorIndex, fBootHead ? 1 : 0, fWidth,
	      fHeight);
	return true;
}

void RaphaelFramebuffer::stop(IOService *provider)
{
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
	if (!fController || !fController->apertureMemory())
		return nullptr;
	IODeviceMemory *bar0 = fController->apertureMemory();
	const IOFBLinearMode *mode = modeById(fCurrentMode);
	IOByteCount need = bar0->getLength();
	if (mode) {
		const IOByteCount modeBytes = (IOByteCount)mode->width * mode->height * 4;
		if (modeBytes > 0 && modeBytes <= bar0->getLength())
			need = modeBytes;
	}
	IODeviceMemory *sub = IODeviceMemory::withSubRange(bar0, 0, need);
	if (sub)
		return sub;
	bar0->retain();
	return bar0;
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
