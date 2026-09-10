#pragma once

#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/graphics/IOFramebuffer.h>

struct IOFBLinearMode {
	IODisplayModeID modeId;
	UInt32 width;
	UInt32 height;
	UInt32 refreshHz;
	UInt32 bytesPerRow; /* 0 = width * 4 */
};

class IOFBLinearShell : public IOFramebuffer {
	OSDeclareDefaultStructors(IOFBLinearShell);

public:
	virtual bool init(OSDictionary *dictionary = 0) APPLE_KEXT_OVERRIDE;
	virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
	virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
	virtual IOWorkLoop *getWorkLoop() const APPLE_KEXT_OVERRIDE;

	virtual IOReturn enableController(void) APPLE_KEXT_OVERRIDE;
	virtual IODeviceMemory *getApertureRange(IOPixelAperture aperture) APPLE_KEXT_OVERRIDE;
	virtual IODeviceMemory *getVRAMRange(void) APPLE_KEXT_OVERRIDE;
	virtual const char *getPixelFormats(void) APPLE_KEXT_OVERRIDE;
	virtual IOItemCount getDisplayModeCount(void) APPLE_KEXT_OVERRIDE;
	virtual IOReturn getDisplayModes(IODisplayModeID *allDisplayModes) APPLE_KEXT_OVERRIDE;
	virtual IOReturn getInformationForDisplayMode(IODisplayModeID displayMode,
						      IODisplayModeInformation *info) APPLE_KEXT_OVERRIDE;
	virtual UInt64 getPixelFormatsForDisplayMode(IODisplayModeID displayMode,
						     IOIndex depth) APPLE_KEXT_OVERRIDE;
	virtual IOReturn getPixelInformation(IODisplayModeID displayMode, IOIndex depth,
					     IOPixelAperture aperture,
					     IOPixelInformation *pixelInfo) APPLE_KEXT_OVERRIDE;
	virtual IOReturn getCurrentDisplayMode(IODisplayModeID *displayMode,
					       IOIndex *depth) APPLE_KEXT_OVERRIDE;
	virtual IOReturn setDisplayMode(IODisplayModeID displayMode, IOIndex depth) APPLE_KEXT_OVERRIDE;
	virtual IOReturn getTimingInfoForDisplayMode(IODisplayModeID displayMode,
						     IOTimingInformation *info) APPLE_KEXT_OVERRIDE;
	virtual IOReturn setGammaTable(UInt32 channelCount, UInt32 dataCount, UInt32 dataWidth,
				       void *data) APPLE_KEXT_OVERRIDE;
	virtual IOReturn setCLUTWithEntries(IOColorEntry *colors, UInt32 index, UInt32 numEntries,
					    IOOptionBits options) APPLE_KEXT_OVERRIDE;
	virtual IOReturn getAttribute(IOSelect attribute, uintptr_t *value) APPLE_KEXT_OVERRIDE;
	virtual IOReturn setAttribute(IOSelect attribute, uintptr_t value) APPLE_KEXT_OVERRIDE;
	virtual IOReturn registerForInterruptType(IOSelect interruptType, IOFBInterruptProc proc,
						  OSObject *target, void *ref,
						  void **interruptRef) APPLE_KEXT_OVERRIDE;
	virtual IOReturn unregisterInterrupt(void *interruptRef) APPLE_KEXT_OVERRIDE;
	virtual IOReturn setInterruptState(void *interruptRef, UInt32 state) APPLE_KEXT_OVERRIDE;

protected:
	virtual IODeviceMemory *copyApertureMemory(void) = 0;
	virtual IODeviceMemory *copyVramMemory(void);
	void setModeTable(const IOFBLinearMode *modes, IOItemCount count, IODisplayModeID currentId);
	const IOFBLinearMode *modeById(IODisplayModeID modeId) const;

	IOWorkLoop *fWorkLoop;
	IOTimerEventSource *fVblTimer;
	IOFBInterruptProc fVblProc;
	OSObject *fVblTarget;
	void *fVblRef;
	bool fVblEnabled;
	IODisplayModeID fCurrentMode;
	IOItemCount fModeCount;
	IOFBLinearMode fModes[8];

	static void vblAction(OSObject *owner, IOTimerEventSource *sender);
};
