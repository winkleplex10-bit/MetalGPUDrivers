#include "RaphaelDcnModeset.h"
#include "RaphaelDcnRegs.h"
#include "RaphaelIds.h"

#include <IOKit/IOLib.h>

struct RaphaelDcnOtgSnap {
	uint32_t ctlOff;
	uint32_t hOff;
	uint32_t vOff;
	uint32_t hbOff;
	uint32_t vbOff;
	uint32_t ctl;
	uint32_t htot;
	uint32_t vtot;
	uint32_t vblank;
	uint32_t hActive;
	uint32_t vActive;
};

static bool raphaelDcnReadOtgSnap(IOMemoryMap *bar5, unsigned otgIndex, RaphaelDcnOtgSnap *snap)
{
	if (!bar5 || !snap || otgIndex >= kRaphaelOtgCount)
		return false;

	snap->ctlOff = kRaphaelDcnSeg2 + kRaphaelOtgControlReg[otgIndex];
	snap->hOff = kRaphaelDcnSeg2 + kRaphaelOtgHTotalReg[otgIndex];
	snap->vOff = kRaphaelDcnSeg2 + kRaphaelOtgVTotalReg[otgIndex];
	snap->hbOff = kRaphaelDcnSeg2 + kRaphaelOtgHBlankReg[otgIndex];
	snap->vbOff = kRaphaelDcnSeg2 + kRaphaelOtgVBlankReg[otgIndex];

	uint32_t hblank = 0, vblank = 0;
	if (!raphaelDcnRead32(bar5, snap->ctlOff, &snap->ctl) ||
	    !raphaelDcnRead32(bar5, snap->hOff, &snap->htot) ||
	    !raphaelDcnRead32(bar5, snap->vOff, &snap->vtot) ||
	    !raphaelDcnRead32(bar5, snap->hbOff, &hblank) ||
	    !raphaelDcnRead32(bar5, snap->vbOff, &vblank))
		return false;
	snap->vblank = vblank;

	const uint32_t hs = hblank & kRaphaelOtgBlankStartMask;
	const uint32_t he = (hblank & kRaphaelOtgBlankEndMask) >> kRaphaelOtgBlankEndShift;
	const uint32_t vs = vblank & kRaphaelOtgBlankStartMask;
	const uint32_t ve = (vblank & kRaphaelOtgBlankEndMask) >> kRaphaelOtgBlankEndShift;
	snap->hActive = (hs >= he) ? (hs - he) : 0;
	snap->vActive = (vs >= ve) ? (vs - ve) : 0;
	return true;
}

static bool raphaelDcnLiveGop4k(const RaphaelDcnOtgSnap *snap)
{
	return snap && snap->hActive == kRaphaelDefaultWidth &&
	       snap->vActive == kRaphaelDefaultHeight &&
	       (snap->ctl & kRaphaelOtgMasterEnMask) != 0;
}

static int raphaelDcnFindHubpForOtg(IOMemoryMap *bar5, unsigned otgIndex, uint32_t *cntlOut)
{
	int fallback = -1;
	uint32_t fallbackCntl = 0;
	for (unsigned i = 0; i < kRaphaelHubpCount; i++) {
		uint32_t cntl = 0;
		const uint32_t off = kRaphaelDcnSeg2 + kRaphaelHubpCntlReg[i];
		if (!raphaelDcnRead32(bar5, off, &cntl))
			continue;
		const unsigned vtg = (cntl & kRaphaelHubpVtgSelMask) >> kRaphaelHubpVtgSelShift;
		if (vtg != otgIndex)
			continue;
		if ((cntl & kRaphaelHubpBlankEnMask) == 0) {
			if (cntlOut)
				*cntlOut = cntl;
			return (int)i;
		}
		if (fallback < 0) {
			fallback = (int)i;
			fallbackCntl = cntl;
		}
	}
	if (fallback >= 0 && cntlOut)
		*cntlOut = fallbackCntl;
	return fallback;
}

static bool raphaelDcnSetHubpBlank(IOMemoryMap *bar5, unsigned hubpIndex, bool blank, uint32_t *after)
{
	const uint32_t off = kRaphaelDcnSeg2 + kRaphaelHubpCntlReg[hubpIndex];
	uint32_t cntl = 0;
	if (!raphaelDcnRead32(bar5, off, &cntl))
		return false;
	if (blank)
		cntl |= kRaphaelHubpBlankEnMask;
	else
		cntl &= ~kRaphaelHubpBlankEnMask;
	if (!raphaelDcnWrite32(bar5, off, cntl))
		return false;
	if (after && !raphaelDcnRead32(bar5, off, after))
		return false;
	return true;
}

bool RaphaelDcnSetLivePipeBlank(IOMemoryMap *bar5, unsigned otgIndex, bool blank)
{
	if (!bar5 || otgIndex >= kRaphaelOtgCount)
		return false;
	uint32_t hubpBefore = 0;
	const int hubp = raphaelDcnFindHubpForOtg(bar5, otgIndex, &hubpBefore);
	if (hubp < 0) {
		/*
		 * Do not blank every HUBP. That is a one-way black screen if
		 * unblank cannot find the same pipe.
		 */
		IOLog("RaphaelDcnModeset: no HUBP with HUBP_VTG_SEL=OTG%u; skip blank=%u "
		      "(GOP wrap kept)\n",
		      otgIndex, blank ? 1 : 0);
		return false;
	}
	uint32_t after = 0;
	if (!raphaelDcnSetHubpBlank(bar5, (unsigned)hubp, blank, &after)) {
		IOLog("RaphaelDcnModeset: HUBP%u blank=%u write failed\n", hubp, blank ? 1 : 0);
		return false;
	}
	IOLog("RaphaelDcnModeset: HUBP%u HUBP_BLANK_EN=%u DCHUBP_CNTL=0x%08x\n", hubp,
	      blank ? 1 : 0, after);
	return true;
}

bool RaphaelDcnReaffirmLiveGop4k(IOMemoryMap *bar5, unsigned otgIndex)
{
	if (!bar5 || otgIndex >= kRaphaelOtgCount) {
		IOLog("RaphaelDcnModeset: bad BAR5/OTG%u\n", otgIndex);
		return false;
	}

	RaphaelDcnOtgSnap snap = {};
	if (!raphaelDcnReadOtgSnap(bar5, otgIndex, &snap)) {
		IOLog("RaphaelDcnModeset: OTG%u unread; skip writes (GOP wrap)\n", otgIndex);
		return false;
	}
	if (snap.hActive != kRaphaelDefaultWidth || snap.vActive != kRaphaelDefaultHeight) {
		IOLog("RaphaelDcnModeset: OTG%u active %ux%u != GOP 3840x2160; skip writes\n",
		      otgIndex, snap.hActive, snap.vActive);
		return false;
	}

	/*
	 * Reaffirm live GOP timing. Same dword values GOP already programmed.
	 * Do not clear MASTER_EN, do not touch missing OPTC_CONTROL, do not
	 * guess HDMI vs DP PHY (physical jack is DP; live pipe is this OTG).
	 */
	const uint32_t ctlSet = snap.ctl | kRaphaelOtgMasterEnMask;
	if (!raphaelDcnWrite32(bar5, snap.hOff, snap.htot) ||
	    !raphaelDcnWrite32(bar5, snap.vOff, snap.vtot) ||
	    !raphaelDcnWrite32(bar5, snap.ctlOff, ctlSet)) {
		IOLog("RaphaelDcnModeset: OTG%u write failed; GOP wrap unchanged\n", otgIndex);
		return false;
	}

	uint32_t ctlAfter = 0, hAfter = 0, vAfter = 0;
	raphaelDcnRead32(bar5, snap.ctlOff, &ctlAfter);
	raphaelDcnRead32(bar5, snap.hOff, &hAfter);
	raphaelDcnRead32(bar5, snap.vOff, &vAfter);
	IOLog("RaphaelDcnModeset: issued OTG%u 3840x2160 reaffirm MASTER_EN=1 H_TOTAL=0x%08x "
	      "V_TOTAL=0x%08x (wrote H=0x%08x V=0x%08x CTL=0x%08x->0x%08x)\n",
	      otgIndex, snap.htot, snap.vtot, hAfter, vAfter, snap.ctl, ctlAfter);
	return true;
}

bool RaphaelDcnBlankUnblankLiveGop4k(IOMemoryMap *bar5, unsigned otgIndex)
{
	if (!bar5 || otgIndex >= kRaphaelOtgCount) {
		IOLog("RaphaelDcnModeset: blank aborted: bad BAR5/OTG%u\n", otgIndex);
		return false;
	}

	RaphaelDcnOtgSnap before = {};
	if (!raphaelDcnReadOtgSnap(bar5, otgIndex, &before) || !raphaelDcnLiveGop4k(&before)) {
		IOLog("RaphaelDcnModeset: blank aborted: OTG%u is not live GOP 3840x2160 "
		      "(MASTER_EN=%u H_TOTAL=0x%08x V_TOTAL=0x%08x active=%ux%u)\n",
		      otgIndex, (before.ctl & kRaphaelOtgMasterEnMask) ? 1 : 0, before.htot,
		      before.vtot, before.hActive, before.vActive);
		return false;
	}

	IOLog("RaphaelDcnModeset: OTG%u before blank H_TOTAL=0x%08x V_TOTAL=0x%08x MASTER_EN=1 "
	      "CTL=0x%08x (no OTG_BLANK_CONTROL in dcn_3_1_5_offset.h; no blank GPINT in "
	      "dmub_cmd.h)\n",
	      otgIndex, before.htot, before.vtot, before.ctl);

	uint32_t hubpBefore = 0;
	const int hubp = raphaelDcnFindHubpForOtg(bar5, otgIndex, &hubpBefore);
	if (hubp < 0) {
		IOLog("RaphaelDcnModeset: no cited OTG blank control in dcn_3_1_5_offset.h; "
		      "no HUBP with HUBP_VTG_SEL=OTG%u; GOP wrap kept\n",
		      otgIndex);
		return false;
	}

	uint32_t blankAfter = 0;
	if (!raphaelDcnSetHubpBlank(bar5, (unsigned)hubp, true, &blankAfter)) {
		IOLog("RaphaelDcnModeset: blank issued failed: HUBP%u DCHUBP_CNTL write; "
		      "reason=hubp_blank_write_failed\n",
		      hubp);
		return false;
	}
	IOLog("RaphaelDcnModeset: blank issued HUBP%u DCHUBP_CNTL=0x%08x->0x%08x "
	      "HUBP_BLANK_EN=1 (cited dcn_3_1_5_sh_mask.h); OTG MASTER_EN left set\n",
	      hubp, hubpBefore, blankAfter);

	/* IOLib IOSleep is milliseconds. 100ms ≈ 6 frames at GOP 60Hz. */
	IOSleep(100);

	uint32_t unblankAfter = 0;
	if (!raphaelDcnSetHubpBlank(bar5, (unsigned)hubp, false, &unblankAfter)) {
		IOLog("RaphaelDcnModeset: unblank issued failed: HUBP%u write; "
		      "reason=hubp_unblank_write_failed; restoring GOP 4K\n",
		      hubp);
		raphaelDcnSetHubpBlank(bar5, (unsigned)hubp, false, nullptr);
		RaphaelDcnReaffirmLiveGop4k(bar5, otgIndex);
		return false;
	}
	IOLog("RaphaelDcnModeset: unblank issued HUBP%u DCHUBP_CNTL=0x%08x HUBP_BLANK_EN=0 "
	      "HUBP_IN_BLANK=%u\n",
	      hubp, unblankAfter, (unblankAfter & kRaphaelHubpInBlankMask) ? 1 : 0);

	RaphaelDcnOtgSnap after = {};
	const bool gotAfter = raphaelDcnReadOtgSnap(bar5, otgIndex, &after);
	const bool masterEn = gotAfter && (after.ctl & kRaphaelOtgMasterEnMask) != 0;
	const bool totalsOk = gotAfter && after.htot == before.htot && after.vtot == before.vtot;
	const bool hubpClear = (unblankAfter & kRaphaelHubpBlankEnMask) == 0;
	IOLog("RaphaelDcnModeset: OTG%u after unblank H_TOTAL=0x%08x V_TOTAL=0x%08x MASTER_EN=%u "
	      "(before H=0x%08x V=0x%08x)\n",
	      otgIndex, gotAfter ? after.htot : 0, gotAfter ? after.vtot : 0, masterEn ? 1 : 0,
	      before.htot, before.vtot);

	if (!masterEn || !totalsOk || !hubpClear) {
		const char *reason = !hubpClear ? "hubp_blank_en_stuck" :
						  (!masterEn ? "master_en_cleared" : "h_v_totals_changed");
		IOLog("RaphaelDcnModeset: unblank did not restore GOP 4K; reason=%s; "
		      "reaffirming live pipe\n",
		      reason);
		raphaelDcnSetHubpBlank(bar5, (unsigned)hubp, false, nullptr);
		RaphaelDcnReaffirmLiveGop4k(bar5, otgIndex);
		return false;
	}

	IOLog("RaphaelDcnModeset: blank/unblank ok OTG%u HUBP%u H_TOTAL=0x%08x V_TOTAL=0x%08x "
	      "MASTER_EN=1\n",
	      otgIndex, hubp, after.htot, after.vtot);
	return true;
}

static void raphaelDcnRestoreGopTotals(IOMemoryMap *bar5, unsigned otgIndex,
				       const RaphaelDcnOtgSnap *before)
{
	if (before) {
		raphaelDcnWrite32(bar5, before->vOff, before->vtot);
		raphaelDcnWrite32(bar5, before->vbOff, before->vblank);
	}
	RaphaelDcnReaffirmLiveGop4k(bar5, otgIndex);
}

static void raphaelDcnRestoreGop4kAndUnblank(IOMemoryMap *bar5, unsigned otgIndex,
					     const RaphaelDcnOtgSnap *before)
{
	raphaelDcnRestoreGopTotals(bar5, otgIndex, before);
	RaphaelDcnSetLivePipeBlank(bar5, otgIndex, false);
}

bool RaphaelDcnProbeLiveGopVTotalPlusOne(IOMemoryMap *bar5, unsigned otgIndex)
{
	if (!bar5 || otgIndex >= kRaphaelOtgCount) {
		IOLog("RaphaelDcnModeset: V_TOTAL+1 aborted: bad BAR5/OTG%u\n", otgIndex);
		return false;
	}

	RaphaelDcnOtgSnap before = {};
	if (!raphaelDcnReadOtgSnap(bar5, otgIndex, &before) || !raphaelDcnLiveGop4k(&before)) {
		IOLog("RaphaelDcnModeset: V_TOTAL+1 aborted: OTG%u is not live GOP 3840x2160 "
		      "(MASTER_EN=%u H_TOTAL=0x%08x V_TOTAL=0x%08x active=%ux%u)\n",
		      otgIndex, (before.ctl & kRaphaelOtgMasterEnMask) ? 1 : 0, before.htot,
		      before.vtot, before.hActive, before.vActive);
		return false;
	}
	if (before.vActive != kRaphaelDefaultHeight) {
		IOLog("RaphaelDcnModeset: V_TOTAL+1 aborted: v_active=%u != %u; GOP wrap kept\n",
		      before.vActive, kRaphaelDefaultHeight);
		return false;
	}

	const uint32_t oldTot = before.vtot & kRaphaelOtgTotalMask;
	if (oldTot >= kRaphaelOtgTotalMask) {
		IOLog("RaphaelDcnModeset: V_TOTAL+1 aborted: V_TOTAL=0x%08x would wrap 15-bit "
		      "field; GOP wrap kept\n",
		      before.vtot);
		return false;
	}

	const uint32_t vs = before.vblank & kRaphaelOtgBlankStartMask;
	const uint32_t ve = (before.vblank & kRaphaelOtgBlankEndMask) >> kRaphaelOtgBlankEndShift;
	if (vs >= kRaphaelOtgBlankStartMask) {
		IOLog("RaphaelDcnModeset: V_TOTAL+1 aborted: V_BLANK_START=0x%x would wrap; "
		      "GOP wrap kept\n",
		      vs);
		return false;
	}

	/*
	 * OTG_V_BLANK_START_END (dcn_3_1_5_sh_mask.h): start in low 15 bits,
	 * end in high 15 bits. Tree encoding: vActive = start - end. VFP+1 is
	 * V_TOTAL+1 plus V_BLANK_START+1. End is rewritten as start - 2160 so
	 * vActive stays kRaphaelDefaultHeight (same dword, cited masks).
	 *
	 * 0.2.9 left these totals live after unblank. That is an OTG-only
	 * timing change: DP MSA / DIG / PHY are not in this slice. A DP sink
	 * can drop lock while MMIO still reads as "success". Probe only:
	 * verify while HUBP is blanked, then restore GOP totals before unblank.
	 */
	const uint32_t vsNew = vs + 1;
	if (vsNew < kRaphaelDefaultHeight) {
		IOLog("RaphaelDcnModeset: V_TOTAL+1 aborted: V_BLANK_START+1=0x%x < v_active; "
		      "GOP wrap kept\n",
		      vsNew);
		return false;
	}
	const uint32_t veNew = vsNew - kRaphaelDefaultHeight;
	const uint32_t newVblank = (before.vblank & ~(kRaphaelOtgBlankStartMask | kRaphaelOtgBlankEndMask)) |
				   (vsNew & kRaphaelOtgBlankStartMask) |
				   ((veNew << kRaphaelOtgBlankEndShift) & kRaphaelOtgBlankEndMask);
	const uint32_t newVtot = (before.vtot & ~kRaphaelOtgTotalMask) |
				 ((oldTot + 1) & kRaphaelOtgTotalMask);

	IOLog("RaphaelDcnModeset: OTG%u V_TOTAL+1 probe (restore GOP before unblank) "
	      "H_TOTAL=0x%08x V_TOTAL=0x%08x V_BLANK=0x%08x START=0x%x END=0x%x "
	      "v_active=%u MASTER_EN=1 CTL=0x%08x (no DP MSA/PHY in this slice)\n",
	      otgIndex, before.htot, before.vtot, before.vblank, vs, ve, before.vActive, before.ctl);

	if (!RaphaelDcnSetLivePipeBlank(bar5, otgIndex, true)) {
		IOLog("RaphaelDcnModeset: V_TOTAL+1 failed: reason=write_failed (HUBP blank); "
		      "GOP wrap kept\n");
		RaphaelDcnSetLivePipeBlank(bar5, otgIndex, false);
		return false;
	}

	if (!raphaelDcnWrite32(bar5, before.vOff, newVtot) ||
	    !raphaelDcnWrite32(bar5, before.vbOff, newVblank)) {
		IOLog("RaphaelDcnModeset: V_TOTAL+1 failed: reason=write_failed; restoring GOP 4K\n");
		raphaelDcnRestoreGop4kAndUnblank(bar5, otgIndex, &before);
		return false;
	}

	/* IOLib IOSleep is milliseconds. 100ms ≈ 6 frames at GOP 60Hz. */
	IOSleep(100);

	RaphaelDcnOtgSnap after = {};
	const bool gotAfter = raphaelDcnReadOtgSnap(bar5, otgIndex, &after);
	const bool masterEn = gotAfter && (after.ctl & kRaphaelOtgMasterEnMask) != 0;
	const bool hOk = gotAfter && after.htot == before.htot;
	const bool vActiveOk = gotAfter && after.vActive == kRaphaelDefaultHeight;
	const bool vTotOk = gotAfter && after.vtot == newVtot;

	IOLog("RaphaelDcnModeset: OTG%u V_TOTAL probe before=0x%08x after=0x%08x v_active=%u "
	      "MASTER_EN=%u H_TOTAL=0x%08x (expect V_TOTAL=0x%08x v_active=%u MASTER_EN=1)\n",
	      otgIndex, before.vtot, gotAfter ? after.vtot : 0, gotAfter ? after.vActive : 0,
	      masterEn ? 1 : 0, gotAfter ? after.htot : 0, newVtot, kRaphaelDefaultHeight);

	if (!gotAfter || !masterEn || !hOk || !vActiveOk || !vTotOk) {
		const char *reason = !gotAfter ? "write_failed" :
				     (!masterEn ? "master_en_cleared" :
				      (!hOk ? "h_total_changed" :
				       (!vActiveOk ? "v_active_changed" : "v_total_unchanged")));
		IOLog("RaphaelDcnModeset: V_TOTAL+1 failed: reason=%s; restoring GOP 4K\n", reason);
		raphaelDcnRestoreGop4kAndUnblank(bar5, otgIndex, &before);
		return false;
	}

	IOLog("RaphaelDcnModeset: V_TOTAL+1 write stuck; restoring GOP 4K before unblank "
	      "(V_BLANK_START 0x%x->0x%x was probe-only)\n",
	      vs, vsNew);
	raphaelDcnRestoreGopTotals(bar5, otgIndex, &before);

	RaphaelDcnOtgSnap restored = {};
	const bool gotRestored = raphaelDcnReadOtgSnap(bar5, otgIndex, &restored);
	if (!gotRestored || restored.vtot != before.vtot || restored.vblank != before.vblank) {
		IOLog("RaphaelDcnModeset: GOP 4K restore did not stick; unblanking anyway\n");
		RaphaelDcnSetLivePipeBlank(bar5, otgIndex, false);
		return false;
	}

	if (!RaphaelDcnSetLivePipeBlank(bar5, otgIndex, false)) {
		IOLog("RaphaelDcnModeset: V_TOTAL+1 unblank failed after GOP restore; "
		      "reason=write_failed\n");
		RaphaelDcnSetLivePipeBlank(bar5, otgIndex, false);
		return false;
	}

	IOLog("RaphaelDcnModeset: V_TOTAL+1 probe ok OTG%u GOP V_TOTAL=0x%08x restored "
	      "v_active=%u MASTER_EN=1 H_TOTAL=0x%08x\n",
	      otgIndex, restored.vtot, restored.vActive, restored.htot);
	return true;
}
