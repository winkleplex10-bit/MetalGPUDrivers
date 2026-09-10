#include "RaphaelController.h"
#include "../display/RaphaelConnectorNub.h"
#include "../submit/RaphaelAccelerator.h"

#include <IOKit/IOLib.h>
#include <libkern/OSByteOrder.h>
#include <pexpert/pexpert.h>

#define super IOService
OSDefineMetaClassAndStructors(RaphaelController, IOService);

bool RaphaelController::init(OSDictionary *dictionary)
{
	if (!super::init(dictionary))
		return false;
	fPci = nullptr;
	fAperture = nullptr;
	fMmio = nullptr;
	fRomMap = nullptr;
	fAccel = nullptr;
	fNubCount = 0;
	fForceAll = false;
	fMetal = false;
	for (int i = 0; i < 3; i++)
		fBarMaps[i] = nullptr;
	for (uint32_t i = 0; i < kRaphaelMaxConnectors; i++)
		fNubs[i] = nullptr;
	bzero(&fConnectors, sizeof(fConnectors));
	return true;
}

RaphaelController *RaphaelController::withProvider(IOService *provider)
{
	OSIterator *it = provider->getClientIterator();
	if (!it)
		return nullptr;
	IOService *obj;
	RaphaelController *found = nullptr;
	while ((obj = OSDynamicCast(IOService, it->getNextObject()))) {
		found = OSDynamicCast(RaphaelController, obj);
		if (found)
			break;
	}
	it->release();
	return found;
}

bool RaphaelController::claimRaphael(IOPCIDevice *pci)
{
	OSData *v = OSDynamicCast(OSData, pci->getProperty("vendor-id"));
	OSData *d = OSDynamicCast(OSData, pci->getProperty("device-id"));
	if (!v || !d || v->getLength() < 2 || d->getLength() < 2)
		return false;
	const uint16_t vendor = OSReadLittleInt16(v->getBytesNoCopy(), 0);
	const uint16_t device = OSReadLittleInt16(d->getBytesNoCopy(), 0);
	return vendor == kRaphaelVendorId && device == kRaphaelDeviceId;
}

IOService *RaphaelController::probe(IOService *provider, SInt32 *score)
{
	IOPCIDevice *pci = OSDynamicCast(IOPCIDevice, provider);
	if (!pci || !claimRaphael(pci))
		return nullptr;
	return super::probe(provider, score);
}

bool RaphaelController::mapBars()
{
	fAperture = fPci->getDeviceMemoryWithIndex(0);
	fMmio = fPci->getDeviceMemoryWithIndex(1);
	if (fAperture)
		fAperture->retain();
	if (fMmio)
		fMmio->retain();
	for (UInt32 i = 0; i < 3; i++)
		fBarMaps[i] = fPci->mapDeviceMemoryWithIndex(i);
	return fAperture != nullptr;
}

bool RaphaelController::findAtomRom(const void **bytes, size_t *length)
{
	const UInt32 romBar = fPci->configRead32(kIOPCIConfigExpansionROMBase);
	if ((romBar & 0xFFFFF800) != 0) {
		fPci->configWrite32(kIOPCIConfigExpansionROMBase, romBar | 1);
		fRomMap = fPci->mapDeviceMemoryWithRegister(kIOPCIConfigExpansionROMBase);
	}
	if (!fRomMap && fBarMaps[2]) {
		fBarMaps[2]->retain();
		fRomMap = fBarMaps[2];
	}
	if (!fRomMap)
		return false;
	*bytes = reinterpret_cast<const void *>(fRomMap->getVirtualAddress());
	*length = (size_t)fRomMap->getLength();
	return *bytes && *length >= 0x50;
}

void RaphaelController::parseConnectors()
{
	const void *rom = nullptr;
	size_t romLen = 0;
	if (findAtomRom(&rom, &romLen)) {
		if (AtomParseConnectors(reinterpret_cast<const uint8_t *>(rom), romLen, &fConnectors)) {
			IOLog("RaphaelController: ATOM parsed %u connector(s)\n",
			      fConnectors.connectorCount);
			return;
		}
		IOLog("RaphaelController: ATOM parse failed, using HDMI+DP(+USB-C) fallback\n");
	} else {
		IOLog("RaphaelController: no PCI VBIOS map, using HDMI+DP(+USB-C) fallback\n");
	}
	AtomFallbackHdmiDpUsbc(&fConnectors);
}

void RaphaelController::publishExtras()
{
	for (uint32_t i = 1; i < fConnectors.connectorCount && fNubCount < kRaphaelMaxConnectors;
	     i++) {
		RaphaelConnectorNub *nub = new RaphaelConnectorNub;
		if (!nub)
			break;
		if (!nub->initWithConnector(this, i, &fConnectors.connectors[i]) ||
		    !nub->attach(this) || !nub->start(this)) {
			nub->release();
			break;
		}
		nub->registerService();
		fNubs[fNubCount++] = nub;
	}
	fAccel = new RaphaelAccelerator;
	if (fAccel && !fAccel->startWithController(this)) {
		fAccel->release();
		fAccel = nullptr;
	}
}

bool RaphaelController::start(IOService *provider)
{
	fPci = OSDynamicCast(IOPCIDevice, provider);
	if (!fPci || !super::start(provider))
		return false;
	if (!claimRaphael(fPci))
		return false;

	int dummy = 0;
	fForceAll = PE_parse_boot_argn("raphael_force_all", &dummy, sizeof(dummy));
	dummy = 0;
	if (PE_parse_boot_argn("raphael_metal", &dummy, sizeof(dummy)))
		fMetal = dummy != 0;
	dummy = 0;
	const bool mapHw = PE_parse_boot_argn("raphael_map", &dummy, sizeof(dummy)) && dummy != 0;

	/*
	 * Default: do not touch PCI command or BARs. GOP / IONDRV already owns
	 * this aperture; setMemoryEnable/mapDeviceMemory hung the 7 Sep 2026
	 * v0.1.2 boot. Hardware map is opt-in via raphael_map=1.
	 */
	if (mapHw) {
		fPci->setMemoryEnable(true);
		fPci->setBusMasterEnable(true);
		if (!mapBars())
			IOLog("RaphaelController: raphael_map=1 but BAR0 missing, using fallback connectors\n");
		parseConnectors();
	} else {
		AtomFallbackHdmiDpUsbc(&fConnectors);
	}
	setName("RaphaelController");
	setProperty("RaphaelVendorId", (UInt32)kRaphaelVendorId, 32);
	setProperty("RaphaelDeviceId", (UInt32)kRaphaelDeviceId, 32);
	setProperty("RaphaelModel", kRaphaelModelName);
	setProperty("RaphaelPhase", "R1-enumerate");
	setProperty("RaphaelMap", mapHw);
	OSArray *names = OSArray::withCapacity(fConnectors.connectorCount);
	for (uint32_t i = 0; i < fConnectors.connectorCount; i++) {
		OSString *s = OSString::withCString(
			RaphaelConnectorKindName(fConnectors.connectors[i].kind));
		if (names && s)
			names->setObject(s);
		if (s)
			s->release();
	}
	if (names) {
		setProperty("RaphaelConnectors", names);
		names->release();
	}
	registerService();
	publishExtras();
	IOLog("RaphaelController: attached 1002:164e connectors=%u metal=%d force_all=%d map=%d\n",
	      fConnectors.connectorCount, fMetal ? 1 : 0, fForceAll ? 1 : 0, mapHw ? 1 : 0);
	return true;
}

void RaphaelController::stop(IOService *provider)
{
	for (uint32_t i = 0; i < fNubCount; i++) {
		if (fNubs[i]) {
			fNubs[i]->terminate();
			fNubs[i]->release();
			fNubs[i] = nullptr;
		}
	}
	fNubCount = 0;
	if (fAccel) {
		fAccel->terminate();
		fAccel->release();
		fAccel = nullptr;
	}
	if (fRomMap) {
		fRomMap->release();
		fRomMap = nullptr;
	}
	for (int i = 0; i < 3; i++) {
		if (fBarMaps[i]) {
			fBarMaps[i]->release();
			fBarMaps[i] = nullptr;
		}
	}
	if (fAperture) {
		fAperture->release();
		fAperture = nullptr;
	}
	if (fMmio) {
		fMmio->release();
		fMmio = nullptr;
	}
	super::stop(provider);
}
