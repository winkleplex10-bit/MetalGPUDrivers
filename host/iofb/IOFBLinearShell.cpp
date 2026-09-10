#include "IOFBLinearShell.h"

#include <IOKit/IOLib.h>
#include <IOKit/IOWorkLoop.h>

#define super IOFramebuffer
OSDefineMetaClassAndAbstractStructors(IOFBLinearShell, IOFramebuffer);

static const char kPixelFormats[] = IO32BitDirectPixels "\0";

bool IOFBLinearShell::init(OSDictionary *dictionary)
{
	if (!super::init(dictionary))
		return false;
	fWorkLoop = nullptr;
	fVblTimer = nullptr;
	fVblProc = nullptr;
	fVblTarget = nullptr;
	fVblRef = nullptr;
	fVblEnabled = false;
	fCurrentMode = 0x00000547;
	fModeCount = 0;
	return true;
}

bool IOFBLinearShell::start(IOService *provider)
{
	if (!super::start(provider))
		return false;
	fWorkLoop = IOWorkLoop::workLoop();
	if (!fWorkLoop)
		return false;
	fVblTimer = IOTimerEventSource::timerEventSource(this, &IOFBLinearShell::vblAction);
	if (!fVblTimer || fWorkLoop->addEventSource(fVblTimer) != kIOReturnSuccess)
		return false;
	if (fModeCount == 0) {
		static const IOFBLinearMode kDefault[] = {
			{ 0x00000547, 3840, 2160, 60 },
		};
		setModeTable(kDefault, 1, 0x00000547);
	}
	return true;
}

void IOFBLinearShell::stop(IOService *provider)
{
	if (fVblTimer) {
		fVblTimer->cancelTimeout();
		if (fWorkLoop)
			fWorkLoop->removeEventSource(fVblTimer);
		fVblTimer->release();
		fVblTimer = nullptr;
	}
	if (fWorkLoop) {
		fWorkLoop->release();
		fWorkLoop = nullptr;
	}
	super::stop(provider);
}

IOWorkLoop *IOFBLinearShell::getWorkLoop() const
{
	return fWorkLoop;
}

void IOFBLinearShell::setModeTable(const IOFBLinearMode *modes, IOItemCount count,
				  IODisplayModeID currentId)
{
	if (count > 8)
		count = 8;
	fModeCount = count;
	for (IOItemCount i = 0; i < count; i++)
		fModes[i] = modes[i];
	fCurrentMode = currentId;
}

const IOFBLinearMode *IOFBLinearShell::modeById(IODisplayModeID modeId) const
{
	for (IOItemCount i = 0; i < fModeCount; i++) {
		if (fModes[i].modeId == modeId)
			return &fModes[i];
	}
	return nullptr;
}

IODeviceMemory *IOFBLinearShell::copyVramMemory(void)
{
	return copyApertureMemory();
}

IOReturn IOFBLinearShell::enableController(void)
{
	return kIOReturnSuccess;
}

IODeviceMemory *IOFBLinearShell::getApertureRange(IOPixelAperture aperture)
{
	if (aperture != kIOFBSystemAperture)
		return nullptr;
	return copyApertureMemory();
}

IODeviceMemory *IOFBLinearShell::getVRAMRange(void)
{
	return copyVramMemory();
}

const char *IOFBLinearShell::getPixelFormats(void)
{
	return kPixelFormats;
}

IOItemCount IOFBLinearShell::getDisplayModeCount(void)
{
	return fModeCount;
}

IOReturn IOFBLinearShell::getDisplayModes(IODisplayModeID *allDisplayModes)
{
	if (!allDisplayModes)
		return kIOReturnBadArgument;
	for (IOItemCount i = 0; i < fModeCount; i++)
		allDisplayModes[i] = fModes[i].modeId;
	return kIOReturnSuccess;
}

IOReturn IOFBLinearShell::getInformationForDisplayMode(IODisplayModeID displayMode,
						       IODisplayModeInformation *info)
{
	const IOFBLinearMode *mode = modeById(displayMode);
	if (!mode || !info)
		return kIOReturnBadArgument;
	bzero(info, sizeof(*info));
	info->nominalWidth = mode->width;
	info->nominalHeight = mode->height;
	info->refreshRate = mode->refreshHz << 16;
	info->maxDepthIndex = 0;
	info->flags = kDisplayModeValidFlag | kDisplayModeSafeFlag;
	return kIOReturnSuccess;
}

UInt64 IOFBLinearShell::getPixelFormatsForDisplayMode(IODisplayModeID, IOIndex)
{
	return 0;
}

IOReturn IOFBLinearShell::getPixelInformation(IODisplayModeID displayMode, IOIndex depth,
					      IOPixelAperture aperture,
					      IOPixelInformation *pixelInfo)
{
	const IOFBLinearMode *mode = modeById(displayMode);
	if (!mode || !pixelInfo || depth != 0 || aperture != kIOFBSystemAperture)
		return kIOReturnBadArgument;
	bzero(pixelInfo, sizeof(*pixelInfo));
	pixelInfo->bytesPerRow = mode->bytesPerRow ? mode->bytesPerRow : mode->width * 4;
	pixelInfo->bytesPerPlane = 0;
	pixelInfo->bitsPerPixel = 32;
	pixelInfo->pixelType = kIORGBDirectPixels;
	pixelInfo->componentCount = 3;
	pixelInfo->bitsPerComponent = 8;
	pixelInfo->componentMasks[0] = 0x00FF0000;
	pixelInfo->componentMasks[1] = 0x0000FF00;
	pixelInfo->componentMasks[2] = 0x000000FF;
	strlcpy(pixelInfo->pixelFormat, IO32BitDirectPixels, sizeof(pixelInfo->pixelFormat));
	pixelInfo->flags = 0;
	pixelInfo->activeWidth = mode->width;
	pixelInfo->activeHeight = mode->height;
	return kIOReturnSuccess;
}

IOReturn IOFBLinearShell::getCurrentDisplayMode(IODisplayModeID *displayMode, IOIndex *depth)
{
	if (!displayMode || !depth)
		return kIOReturnBadArgument;
	*displayMode = fCurrentMode;
	*depth = 0;
	return kIOReturnSuccess;
}

IOReturn IOFBLinearShell::setDisplayMode(IODisplayModeID displayMode, IOIndex depth)
{
	if (depth != 0 || !modeById(displayMode))
		return kIOReturnUnsupported;
	fCurrentMode = displayMode;
	return kIOReturnSuccess;
}

IOReturn IOFBLinearShell::getTimingInfoForDisplayMode(IODisplayModeID displayMode,
						      IOTimingInformation *info)
{
	const IOFBLinearMode *mode = modeById(displayMode);
	if (!mode || !info)
		return kIOReturnBadArgument;
	bzero(info, sizeof(*info));
	info->appleTimingID = 0;
	info->flags = 0;
	return kIOReturnSuccess;
}

IOReturn IOFBLinearShell::setGammaTable(UInt32, UInt32, UInt32, void *)
{
	return kIOReturnSuccess;
}

IOReturn IOFBLinearShell::setCLUTWithEntries(IOColorEntry *, UInt32, UInt32, IOOptionBits)
{
	return kIOReturnSuccess;
}

IOReturn IOFBLinearShell::getAttribute(IOSelect attribute, uintptr_t *value)
{
	if (!value)
		return kIOReturnBadArgument;
	switch (attribute) {
	case kIOHardwareCursorAttribute:
		*value = 0;
		return kIOReturnSuccess;
	default:
		return super::getAttribute(attribute, value);
	}
}

IOReturn IOFBLinearShell::setAttribute(IOSelect attribute, uintptr_t value)
{
	return super::setAttribute(attribute, value);
}

void IOFBLinearShell::vblAction(OSObject *owner, IOTimerEventSource *sender)
{
	IOFBLinearShell *self = OSDynamicCast(IOFBLinearShell, owner);
	if (!self)
		return;
	if (self->fVblEnabled && self->fVblProc)
		self->fVblProc(self->fVblTarget, self->fVblRef);
	if (self->fVblEnabled)
		sender->setTimeoutMS(16);
}

IOReturn IOFBLinearShell::registerForInterruptType(IOSelect interruptType, IOFBInterruptProc proc,
						   OSObject *target, void *ref, void **interruptRef)
{
	if (interruptType != kIOFBVBLInterruptType || !interruptRef)
		return kIOReturnUnsupported;
	fVblProc = proc;
	fVblTarget = target;
	fVblRef = ref;
	*interruptRef = this;
	return kIOReturnSuccess;
}

IOReturn IOFBLinearShell::unregisterInterrupt(void *interruptRef)
{
	if (interruptRef != this)
		return kIOReturnBadArgument;
	fVblEnabled = false;
	fVblProc = nullptr;
	if (fVblTimer)
		fVblTimer->cancelTimeout();
	return kIOReturnSuccess;
}

IOReturn IOFBLinearShell::setInterruptState(void *interruptRef, UInt32 state)
{
	if (interruptRef != this)
		return kIOReturnBadArgument;
	fVblEnabled = state != 0;
	if (fVblEnabled && fVblTimer)
		fVblTimer->setTimeoutMS(16);
	else if (fVblTimer)
		fVblTimer->cancelTimeout();
	return kIOReturnSuccess;
}
