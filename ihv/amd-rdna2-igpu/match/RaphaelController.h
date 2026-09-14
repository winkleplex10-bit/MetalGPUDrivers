#pragma once

#include "RaphaelIds.h"
#include "atom_parse.h"

#include <IOKit/IOLocks.h>
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
	bool dcnModesetRequested() const { return fDcnModeset; }
	bool dcnHandshakeOk() const { return fDmubOk; }
	uint32_t bootConnectorIndex() const;
	RaphaelConnector bootConnectorSpec() const;

	/* After GOP wrap is alive. No-op unless raphael_dcn_probe=1 or
	 * raphael_dcn_dump=1 (dump implies probe). Read-only BAR5. */
	void probeDcnBar5IfRequested();
	/* raphael_dcn_modeset=1 only: DMUB handshake + gated OTG0 4K writes. */
	bool runHwModesetIfRequested();

	static RaphaelController *withProvider(IOService *provider);

private:
	bool claimRaphael(IOPCIDevice *pci);
	bool mapBars();
	bool mapBar5();
	void unmapBar5(const char *why);
	void applyConnectorFallback();
	void publishExtras();
	void dumpDcnLivePipe();
	void discoverLivePipe();
	void logLiveHpdSense(const char *when);

	IOPCIDevice *fPci;
	IOMemoryMap *fBarMaps[3];
	IOMemoryMap *fBar5Map;
	IODeviceMemory *fAperture;
	IODeviceMemory *fMmio;
	AtomParseResult fConnectors;
	RaphaelAccelerator *fAccel;
	RaphaelConnectorNub *fNubs[kRaphaelMaxConnectors];
	uint32_t fNubCount;
	int fLiveOtg;
	int fLiveHpd;
	bool fForceAll;
	bool fMetal;
	bool fDcnProbe;
	bool fDcnDump;
	bool fDcnModeset;
	bool fDcnVtotal;
	bool fDmubOk;
	bool fHwModesetIssued;
	IOLock *fDcnLock;
};
