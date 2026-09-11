#include "RaphaelController.h"
#include "../display/RaphaelConnectorNub.h"
#include "../display/RaphaelDcnModeset.h"
#include "../display/RaphaelDcnRegs.h"
#include "../display/RaphaelDmub.h"
#include "../submit/RaphaelAccelerator.h"

#include <IOKit/IODeviceMemory.h>
#include <IOKit/IOMemoryDescriptor.h>
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
	fAccel = nullptr;
	fNubCount = 0;
	fLiveOtg = 0;
	fLiveHpd = 2;
	fForceAll = false;
	fMetal = false;
	fDcnProbe = false;
	fDcnDump = false;
	fDcnModeset = false;
	fDmubOk = false;
	fHwModesetIssued = false;
	fBar5Map = nullptr;
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
	/*
	 * raphael_map=1 only. Index 0 is BAR0 (GOP FB). Index 1 is BAR2
	 * doorbells, not DCN MMIO. DCN 3.1.5 is PCI BAR5 (cfg 0x24, 512KB).
	 */
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

void RaphaelController::applyConnectorFallback()
{
	if (fForceAll) {
		AtomFallbackHdmiDpUsbc(&fConnectors);
		IOLog("RaphaelController: raphael_force_all=1, HDMI+DP+USB-C fallback\n");
	} else {
		AtomFallbackHdmiDp(&fConnectors);
		IOLog("RaphaelController: VFCT HDMI-A then DP objects; live jack is physical DP "
		      "(user); ATOM names may disagree with silkscreen\n");
	}
}

uint32_t RaphaelController::bootConnectorIndex() const
{
	/*
	 * Lab cable is DisplayPort. VFCT path 0 is HDMI-A 0x320C; path 2 is DP
	 * 0x3113. Do not treat path 0 as the boot head.
	 */
	for (uint32_t i = 0; i < fConnectors.connectorCount; i++) {
		if (fConnectors.connectors[i].kind == kRaphaelConnectorDp)
			return i;
	}
	return 0;
}

RaphaelConnector RaphaelController::bootConnectorSpec() const
{
	const uint32_t idx = bootConnectorIndex();
	if (idx < fConnectors.connectorCount)
		return fConnectors.connectors[idx];
	RaphaelConnector spec = {};
	spec.kind = kRaphaelConnectorDp;
	spec.preferOnline = true;
	return spec;
}

bool RaphaelController::mapBar5()
{
	if (fBar5Map)
		return true;
	if (!fPci)
		return false;

	IODeviceMemory *bar5 = fPci->getDeviceMemoryWithRegister(kIOPCIConfigBaseAddress5);
	if (!bar5) {
		IOLog("RaphaelController: BAR5 cfg 0x24 missing\n");
		return false;
	}

	const IOByteCount len = bar5->getLength();
	IOByteCount segLen = 0;
	const addr64_t phys = bar5->getPhysicalSegment(0, &segLen, kIOMemoryMapperNone);

	if (len == 0 || len >= kRaphaelBar2Bytes) {
		IOLog("RaphaelController: BAR5 cfg 0x24 phys=0x%llx size=%llu not 512KB; skip map\n",
		      (unsigned long long)phys, (unsigned long long)len);
		return false;
	}
	if (len != kRaphaelBar5Expected)
		IOLog("RaphaelController: BAR5 size=%llu (lab 524288)\n", (unsigned long long)len);

	/* Memory Enable already on for GOP — do not setMemoryEnable / setBusMasterEnable. */
	fBar5Map = bar5->map();
	IOLog("RaphaelController: BAR5 phys=0x%llx size=%llu map=%d\n", (unsigned long long)phys,
	      (unsigned long long)len, fBar5Map ? 1 : 0);
	if (!fBar5Map) {
		IOLog("RaphaelController: BAR5 map failed\n");
		return false;
	}
	return true;
}

void RaphaelController::discoverLivePipe()
{
	if (!fBar5Map)
		return;

	int senseHpd = -1;
	for (unsigned i = 0; i < sizeof(kRaphaelHpdIntStatusReg) / sizeof(kRaphaelHpdIntStatusReg[0]);
	     i++) {
		uint32_t raw = 0;
		const uint32_t dwordOff = kRaphaelDcnSeg2 + kRaphaelHpdIntStatusReg[i];
		if (raphaelDcnRead32(fBar5Map, dwordOff, &raw) && (raw & kRaphaelHpdSenseMask)) {
			senseHpd = (int)i;
			break;
		}
	}
	if (senseHpd >= 0)
		fLiveHpd = senseHpd;

	int liveOtg = -1;
	int matchOtg = -1;
	for (unsigned i = 0; i < kRaphaelOtgCount; i++) {
		uint32_t ctl = 0, hblank = 0, vblank = 0;
		const uint32_t ctlOff = kRaphaelDcnSeg2 + kRaphaelOtgControlReg[i];
		const uint32_t hbOff = kRaphaelDcnSeg2 + kRaphaelOtgHBlankReg[i];
		const uint32_t vbOff = kRaphaelDcnSeg2 + kRaphaelOtgVBlankReg[i];
		if (!raphaelDcnRead32(fBar5Map, ctlOff, &ctl))
			continue;
		const uint32_t masterEn = (ctl & kRaphaelOtgMasterEnMask) ? 1 : 0;
		const uint32_t curEn = (ctl & kRaphaelOtgCurMasterEnMask) ? 1 : 0;
		uint32_t hActive = 0, vActive = 0;
		if (raphaelDcnRead32(fBar5Map, hbOff, &hblank)) {
			const uint32_t hs = hblank & kRaphaelOtgBlankStartMask;
			const uint32_t he = (hblank & kRaphaelOtgBlankEndMask) >> kRaphaelOtgBlankEndShift;
			if (hs >= he)
				hActive = hs - he;
		}
		if (raphaelDcnRead32(fBar5Map, vbOff, &vblank)) {
			const uint32_t vs = vblank & kRaphaelOtgBlankStartMask;
			const uint32_t ve = (vblank & kRaphaelOtgBlankEndMask) >> kRaphaelOtgBlankEndShift;
			if (vs >= ve)
				vActive = vs - ve;
		}
		const unsigned matchGop =
			(hActive == kRaphaelDefaultWidth && vActive == kRaphaelDefaultHeight) ? 1 : 0;
		if (liveOtg < 0 && (masterEn || curEn))
			liveOtg = (int)i;
		if (matchOtg < 0 && matchGop)
			matchOtg = (int)i;
	}
	if (liveOtg < 0)
		liveOtg = matchOtg;
	if (liveOtg >= 0)
		fLiveOtg = liveOtg;

	IOLog("RaphaelController: live pipe OTG%u HPD%u SENSE (physical DP jack; VFCT HDMI-A is "
	      "not the cabled head)\n",
	      fLiveOtg, fLiveHpd);
}

void RaphaelController::probeDcnBar5IfRequested()
{
	if (!fDcnProbe || !fPci)
		return;
	if (!mapBar5())
		return;

	IOLog("RaphaelController: DCN HPD GPIO pins=6 (hw_factory_dcn315); INT_STATUS engines=5; "
	      "DC_HPD_CONNECTED not in dcn_3_1_5_sh_mask.h\n");
	for (unsigned i = 0; i < sizeof(kRaphaelHpdIntStatusReg) / sizeof(kRaphaelHpdIntStatusReg[0]);
	     i++) {
		const uint32_t dwordOff = kRaphaelDcnSeg2 + kRaphaelHpdIntStatusReg[i];
		uint32_t raw = 0;
		if (raphaelDcnRead32(fBar5Map, dwordOff, &raw))
			IOLog("RaphaelController: HPD%u_DC_HPD_INT_STATUS dword=0x%x raw=0x%08x "
			      "INT_STATUS=%u SENSE=%u SENSE_DELAYED=%u RX_INT=%u CONNECTED=not_in_sh_mask\n",
			      i, dwordOff, raw, (raw & kRaphaelHpdIntStatusMask) ? 1 : 0,
			      (raw & kRaphaelHpdSenseMask) ? 1 : 0,
			      (raw & kRaphaelHpdSenseDelayedMask) ? 1 : 0,
			      (raw & kRaphaelHpdRxIntMask) ? 1 : 0);
		else
			IOLog("RaphaelController: HPD%u INT_STATUS dword=0x%x out of BAR5\n", i,
			      dwordOff);
	}
	uint32_t gpioY = 0;
	const uint32_t gpioOff = kRaphaelDcnSeg2 + kRaphaelDcGpioHpdY;
	if (raphaelDcnRead32(fBar5Map, gpioOff, &gpioY))
		IOLog("RaphaelController: DC_GPIO_HPD_Y dword=0x%x raw=0x%08x "
		      "HPD1_Y=%u HPD2_Y=%u HPD3_Y=%u HPD4_Y=%u HPD5_Y=%u HPD6_Y=%u\n",
		      gpioOff, gpioY, (gpioY & kRaphaelGpioHpdYMask[0]) ? 1 : 0,
		      (gpioY & kRaphaelGpioHpdYMask[1]) ? 1 : 0,
		      (gpioY & kRaphaelGpioHpdYMask[2]) ? 1 : 0,
		      (gpioY & kRaphaelGpioHpdYMask[3]) ? 1 : 0,
		      (gpioY & kRaphaelGpioHpdYMask[4]) ? 1 : 0,
		      (gpioY & kRaphaelGpioHpdYMask[5]) ? 1 : 0);
	else
		IOLog("RaphaelController: DC_GPIO_HPD_Y dword=0x%x out of BAR5\n", gpioOff);

	dumpDcnLivePipe();
	discoverLivePipe();
}

void RaphaelController::dumpDcnLivePipe()
{
	if (!fBar5Map)
		return;

	IOLog("RaphaelController: DCN dump TGs=%u (dcn315_resource.c num_timing_generator); "
	      "OPTC_CONTROL not in dcn_3_1_5_offset.h - using ODM*_OPTC_INPUT_CLOCK_CONTROL\n",
	      kRaphaelOtgCount);

	int liveOtg = -1;
	int matchOtg = -1;
	for (unsigned i = 0; i < kRaphaelOtgCount; i++) {
		uint32_t ctl = 0, htot = 0, vtot = 0, hblank = 0, vblank = 0, odmClk = 0;
		const uint32_t ctlOff = kRaphaelDcnSeg2 + kRaphaelOtgControlReg[i];
		const uint32_t hOff = kRaphaelDcnSeg2 + kRaphaelOtgHTotalReg[i];
		const uint32_t vOff = kRaphaelDcnSeg2 + kRaphaelOtgVTotalReg[i];
		const uint32_t hbOff = kRaphaelDcnSeg2 + kRaphaelOtgHBlankReg[i];
		const uint32_t vbOff = kRaphaelDcnSeg2 + kRaphaelOtgVBlankReg[i];
		const uint32_t odmOff = kRaphaelDcnSeg2 + kRaphaelOdmOptcClkReg[i];
		const bool gotCtl = raphaelDcnRead32(fBar5Map, ctlOff, &ctl);
		const bool gotH = raphaelDcnRead32(fBar5Map, hOff, &htot);
		const bool gotV = raphaelDcnRead32(fBar5Map, vOff, &vtot);
		const bool gotHb = raphaelDcnRead32(fBar5Map, hbOff, &hblank);
		const bool gotVb = raphaelDcnRead32(fBar5Map, vbOff, &vblank);
		const bool gotOdm = raphaelDcnRead32(fBar5Map, odmOff, &odmClk);
		if (!gotCtl || !gotH || !gotV) {
			IOLog("RaphaelController: OTG%u dword ctl=0x%x out of BAR5 or unread\n", i,
			      ctlOff);
			continue;
		}

		const uint32_t masterEn = (ctl & kRaphaelOtgMasterEnMask) ? 1 : 0;
		const uint32_t curEn = (ctl & kRaphaelOtgCurMasterEnMask) ? 1 : 0;
		const uint32_t hTotal = htot & kRaphaelOtgTotalMask;
		const uint32_t vTotal = vtot & kRaphaelOtgTotalMask;
		uint32_t hActive = 0;
		uint32_t vActive = 0;
		if (gotHb) {
			const uint32_t hs = hblank & kRaphaelOtgBlankStartMask;
			const uint32_t he = (hblank & kRaphaelOtgBlankEndMask) >> kRaphaelOtgBlankEndShift;
			if (hs >= he)
				hActive = hs - he;
		}
		if (gotVb) {
			const uint32_t vs = vblank & kRaphaelOtgBlankStartMask;
			const uint32_t ve = (vblank & kRaphaelOtgBlankEndMask) >> kRaphaelOtgBlankEndShift;
			if (vs >= ve)
				vActive = vs - ve;
		}
		const unsigned matchGop =
			(hActive == kRaphaelDefaultWidth && vActive == kRaphaelDefaultHeight) ? 1 : 0;
		const unsigned clkEn = gotOdm && (odmClk & kRaphaelOptcClkEnMask) ? 1 : 0;
		const unsigned clkOn = gotOdm && (odmClk & kRaphaelOptcClkOnMask) ? 1 : 0;

		IOLog("RaphaelController: OTG%u OTG_CONTROL dword=0x%x raw=0x%08x MASTER_EN=%u "
		      "CURRENT_MASTER_EN=%u H_TOTAL raw=0x%08x=%u V_TOTAL raw=0x%08x=%u "
		      "h_active=%u v_active=%u match_gop=%u\n",
		      i, ctlOff, ctl, masterEn, curEn, htot, hTotal, vtot, vTotal, hActive, vActive,
		      matchGop);
		if (gotOdm)
			IOLog("RaphaelController: ODM%u OPTC_INPUT_CLOCK_CONTROL dword=0x%x "
			      "raw=0x%08x CLK_EN=%u CLK_ON=%u\n",
			      i, odmOff, odmClk, clkEn, clkOn);
		else
			IOLog("RaphaelController: ODM%u OPTC_INPUT_CLOCK_CONTROL dword=0x%x out of BAR5\n",
			      i, odmOff);

		if (liveOtg < 0 && (masterEn || curEn))
			liveOtg = (int)i;
		if (matchOtg < 0 && matchGop)
			matchOtg = (int)i;
	}

	if (liveOtg < 0)
		liveOtg = matchOtg;

	if (liveOtg >= 0)
		IOLog("RaphaelController: DCN live pipe OTG%u (enabled and/or 3840x2160 active); "
		      "read-only dump\n",
		      liveOtg);
	else
		IOLog("RaphaelController: DCN live pipe: no OTG MASTER_EN / 3840x2160 active in "
		      "OTG0-%u\n",
		      kRaphaelOtgCount - 1);
}

bool RaphaelController::runHwModesetIfRequested()
{
	if (!fDcnModeset)
		return false;
	if (fHwModesetIssued)
		return fDmubOk;

	if (!mapBar5()) {
		IOLog("RaphaelController: DCN modeset aborted: BAR5 map failed; GOP wrap\n");
		fHwModesetIssued = true;
		return false;
	}
	fHwModesetIssued = true;
	discoverLivePipe();

	RaphaelDmubStatus dmub = {};
	fDmubOk = RaphaelDmubLoadAndHandshake(fBar5Map, &dmub);
	setProperty("RaphaelDmubOk", fDmubOk);
	if (!fDmubOk) {
		IOLog("RaphaelController: DCN modeset aborted: %s; GOP wrap kept\n",
		      dmub.failReason ? dmub.failReason : "DMUB handshake failed");
		return false;
	}

	const unsigned otg = (fLiveOtg >= 0) ? (unsigned)fLiveOtg : 0;
	IOLog("RaphaelController: DCN modeset targeting physical DP, VFCT DP object, OTG%u HPD%u "
	      "3840x2160 (HDMI-A VFCT name is not this jack)\n",
	      otg, fLiveHpd);
	if (!RaphaelDcnReaffirmLiveGop4k(fBar5Map, otg)) {
		IOLog("RaphaelController: DCN modeset writes failed; GOP wrap kept\n");
		return false;
	}
	setProperty("RaphaelPhase", "R2-dcn-modeset");
	IOLog("RaphaelController: DCN modeset writes issued on live GOP pipe\n");
	return true;
}

void RaphaelController::publishExtras()
{
	const uint32_t boot = bootConnectorIndex();
	for (uint32_t i = 0; i < fConnectors.connectorCount && fNubCount < kRaphaelMaxConnectors;
	     i++) {
		if (i == boot)
			continue;
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
	dummy = 0;
	fDcnDump = PE_parse_boot_argn("raphael_dcn_dump", &dummy, sizeof(dummy)) && dummy != 0;
	dummy = 0;
	fDcnProbe = (PE_parse_boot_argn("raphael_dcn_probe", &dummy, sizeof(dummy)) && dummy != 0) ||
		    fDcnDump;
	dummy = 0;
	fDcnModeset = PE_parse_boot_argn("raphael_dcn_modeset", &dummy, sizeof(dummy)) && dummy != 0;

	/*
	 * Default: do not touch PCI command or BARs. GOP / IONDRV already owns
	 * this aperture; setMemoryEnable/mapDeviceMemory hung the 7 Sep 2026
	 * v0.1.2 boot. Hardware map is opt-in via raphael_map=1 (leave off).
	 * BAR5 probe is separate (raphael_dcn_probe=1 or raphael_dcn_dump=1).
	 * raphael_dcn_modeset=1 maps BAR5 after GOP wrap and may write OTG0.
	 */
	if (mapHw) {
		fPci->setMemoryEnable(true);
		fPci->setBusMasterEnable(true);
		if (!mapBars())
			IOLog("RaphaelController: raphael_map=1 but BAR0 missing\n");
	}
	applyConnectorFallback();
	setName("RaphaelController");
	setProperty("RaphaelVendorId", (UInt32)kRaphaelVendorId, 32);
	setProperty("RaphaelDeviceId", (UInt32)kRaphaelDeviceId, 32);
	setProperty("RaphaelModel", kRaphaelModelName);
	setProperty("RaphaelPhase", "R1-enumerate");
	setProperty("RaphaelMap", mapHw);
	setProperty("RaphaelDcnProbe", fDcnProbe);
	setProperty("RaphaelDcnDump", fDcnDump);
	setProperty("RaphaelDcnModeset", fDcnModeset);
	setProperty("RaphaelLiveJack", "DP");
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
	IOLog("RaphaelController: attached 1002:164e connectors=%u metal=%d force_all=%d map=%d "
	      "dcn_probe=%d dcn_dump=%d dcn_modeset=%d boot_jack=DP\n",
	      fConnectors.connectorCount, fMetal ? 1 : 0, fForceAll ? 1 : 0, mapHw ? 1 : 0,
	      fDcnProbe ? 1 : 0, fDcnDump ? 1 : 0, fDcnModeset ? 1 : 0);
	if (fDcnModeset)
		IOLog("RaphaelController: raphael_dcn_modeset=1 — DMUB+OTG writes after GOP wrap "
		      "(not license UNKNOWN)\n");
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
	if (fBar5Map) {
		fBar5Map->release();
		fBar5Map = nullptr;
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
