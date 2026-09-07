#pragma once

#include "NvidiaIds.h"

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>

// Phase N1 enumerate-only controller.
// Claims VID/DID allow-list under IOMatchCategory NvidiaHW (does not steal IOFramebuffer).
// Maps BARs for sizing/logging only. Does not boot GSP, does not poke 3D MMIO.

class NvidiaController : public IOService {
	OSDeclareDefaultStructors(NvidiaController);

public:
	virtual bool init(OSDictionary *dictionary = 0) APPLE_KEXT_OVERRIDE;
	virtual IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;
	virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
	virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

	IOPCIDevice *pciDevice() const { return fPci; }

private:
	bool claimAllowlisted(IOPCIDevice *pci, UInt16 *outDevice);
	bool mapBars();
	void publishIdentity(UInt16 deviceId);

	IOPCIDevice *fPci;
	IODeviceMemory *fBars[kNvidiaMaxBars];
	UInt32 fBarCount;
	UInt16 fDeviceId;
	bool fMapBars;
};
