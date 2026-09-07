#pragma once

#include "RaphaelIds.h"
#include "atom_parse.h"

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>

class RaphaelAccelerator;
class RaphaelConnectorNub;

class RaphaelController : public IOService {
	OSDeclareDefaultStructors(RaphaelController);

public:
	virtual bool init(OSDictionary *dictionary = 0) APPLE_KEXT_OVERRIDE;
	virtual IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;
	virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
	virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

	IOPCIDevice *pciDevice() const { return fPci; }
	IODeviceMemory *apertureMemory() const { return fAperture; }
	IODeviceMemory *mmioMemory() const { return fMmio; }
	const AtomParseResult &connectors() const { return fConnectors; }
	bool forceAllConnectors() const { return fForceAll; }
	bool metalEnabled() const { return fMetal; }

	static RaphaelController *withProvider(IOService *provider);

private:
	bool claimRaphael(IOPCIDevice *pci);
	bool mapBars();
	void parseConnectors();
	void publishExtras();
	bool findAtomRom(const void **bytes, size_t *length);

	IOPCIDevice *fPci;
	IOMemoryMap *fBarMaps[3];
	IODeviceMemory *fAperture;
	IODeviceMemory *fMmio;
	IOMemoryMap *fRomMap;
	AtomParseResult fConnectors;
	RaphaelAccelerator *fAccel;
	RaphaelConnectorNub *fNubs[kRaphaelMaxConnectors];
	uint32_t fNubCount;
	bool fForceAll;
	bool fMetal;
};
