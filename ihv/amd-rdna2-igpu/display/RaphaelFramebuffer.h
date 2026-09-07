#pragma once

#include "IOFBLinearShell.h"
#include "RaphaelIds.h"

class RaphaelController;

class RaphaelFramebuffer : public IOFBLinearShell {
	OSDeclareDefaultStructors(RaphaelFramebuffer);

public:
	virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
	virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
	virtual bool isConsoleDevice(void) APPLE_KEXT_OVERRIDE;
	virtual IOReturn enableController(void) APPLE_KEXT_OVERRIDE;
	virtual IOReturn getAttributeForConnection(IOIndex connectIndex, IOSelect attribute,
						   uintptr_t *value) APPLE_KEXT_OVERRIDE;
	virtual IOReturn setAttributeForConnection(IOIndex connectIndex, IOSelect attribute,
						   uintptr_t value) APPLE_KEXT_OVERRIDE;
	virtual IOReturn connectFlags(IOIndex connectIndex, IODisplayModeID displayMode,
				      IOOptionBits *flags) APPLE_KEXT_OVERRIDE;
	virtual bool hasDDCConnect(IOIndex connectIndex) APPLE_KEXT_OVERRIDE;

protected:
	virtual IODeviceMemory *copyApertureMemory(void) APPLE_KEXT_OVERRIDE;

private:
	void applyGopMode(void);
	void releaseController(void);

	RaphaelController *fController;
	uint32_t fConnectorIndex;
	RaphaelConnector fSpec;
	bool fBootHead;
	UInt32 fWidth;
	UInt32 fHeight;
};
