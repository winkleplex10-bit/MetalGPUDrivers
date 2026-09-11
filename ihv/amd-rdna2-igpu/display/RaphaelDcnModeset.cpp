#include "RaphaelDcnModeset.h"
#include "RaphaelDcnRegs.h"
#include "RaphaelIds.h"

#include <IOKit/IOLib.h>

bool RaphaelDcnReaffirmLiveGop4k(IOMemoryMap *bar5, unsigned otgIndex)
{
	if (!bar5 || otgIndex >= kRaphaelOtgCount) {
		IOLog("RaphaelDcnModeset: bad BAR5/OTG%u\n", otgIndex);
		return false;
	}

	const uint32_t ctlOff = kRaphaelDcnSeg2 + kRaphaelOtgControlReg[otgIndex];
	const uint32_t hOff = kRaphaelDcnSeg2 + kRaphaelOtgHTotalReg[otgIndex];
	const uint32_t vOff = kRaphaelDcnSeg2 + kRaphaelOtgVTotalReg[otgIndex];
	const uint32_t hbOff = kRaphaelDcnSeg2 + kRaphaelOtgHBlankReg[otgIndex];
	const uint32_t vbOff = kRaphaelDcnSeg2 + kRaphaelOtgVBlankReg[otgIndex];

	uint32_t ctl = 0, htot = 0, vtot = 0, hblank = 0, vblank = 0;
	if (!raphaelDcnRead32(bar5, ctlOff, &ctl) || !raphaelDcnRead32(bar5, hOff, &htot) ||
	    !raphaelDcnRead32(bar5, vOff, &vtot) || !raphaelDcnRead32(bar5, hbOff, &hblank) ||
	    !raphaelDcnRead32(bar5, vbOff, &vblank)) {
		IOLog("RaphaelDcnModeset: OTG%u unread; skip writes (GOP wrap)\n", otgIndex);
		return false;
	}

	const uint32_t hs = hblank & kRaphaelOtgBlankStartMask;
	const uint32_t he = (hblank & kRaphaelOtgBlankEndMask) >> kRaphaelOtgBlankEndShift;
	const uint32_t vs = vblank & kRaphaelOtgBlankStartMask;
	const uint32_t ve = (vblank & kRaphaelOtgBlankEndMask) >> kRaphaelOtgBlankEndShift;
	const uint32_t hActive = (hs >= he) ? (hs - he) : 0;
	const uint32_t vActive = (vs >= ve) ? (vs - ve) : 0;
	if (hActive != kRaphaelDefaultWidth || vActive != kRaphaelDefaultHeight) {
		IOLog("RaphaelDcnModeset: OTG%u active %ux%u != GOP 3840x2160; skip writes\n",
		      otgIndex, hActive, vActive);
		return false;
	}

	/*
	 * Reaffirm live GOP timing. Same dword values GOP already programmed.
	 * Do not clear MASTER_EN, do not touch missing OPTC_CONTROL, do not
	 * guess HDMI vs DP PHY (physical jack is DP; live pipe is this OTG).
	 */
	const uint32_t ctlSet = ctl | kRaphaelOtgMasterEnMask;
	if (!raphaelDcnWrite32(bar5, hOff, htot) || !raphaelDcnWrite32(bar5, vOff, vtot) ||
	    !raphaelDcnWrite32(bar5, ctlOff, ctlSet)) {
		IOLog("RaphaelDcnModeset: OTG%u write failed; GOP wrap unchanged\n", otgIndex);
		return false;
	}

	uint32_t ctlAfter = 0, hAfter = 0, vAfter = 0;
	raphaelDcnRead32(bar5, ctlOff, &ctlAfter);
	raphaelDcnRead32(bar5, hOff, &hAfter);
	raphaelDcnRead32(bar5, vOff, &vAfter);
	IOLog("RaphaelDcnModeset: issued OTG%u 3840x2160 reaffirm MASTER_EN=1 H_TOTAL=0x%08x "
	      "V_TOTAL=0x%08x (wrote H=0x%08x V=0x%08x CTL=0x%08x->0x%08x)\n",
	      otgIndex, htot, vtot, hAfter, vAfter, ctl, ctlAfter);
	return true;
}
