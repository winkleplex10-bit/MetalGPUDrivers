#pragma once

#include <IOKit/IOMemoryDescriptor.h>
#include <stdint.h>

/*
 * Hardware modeset on the live GOP 3840×2160 pipe (OTG0 / physical DP).
 * DCN writes only after DMUB handshake. Do not invent PHY/PLL offsets
 * missing from dcn_3_1_5_offset.h. Do not invent OTG_BLANK_CONTROL.
 *
 * Boot path (raphael_dcn_modeset=1): reaffirm + HUBP blank/unblank only.
 * V_TOTAL+1 is opt-in (raphael_dcn_vtotal=1). Leaving non-GOP totals on a
 * DP sink can drop the link with no cited MSA/PHY recovery.
 */
bool RaphaelDcnReaffirmLiveGop4k(IOMemoryMap *bar5, unsigned otgIndex);
bool RaphaelDcnBlankUnblankLiveGop4k(IOMemoryMap *bar5, unsigned otgIndex);
/* Cited HUBP_BLANK_EN on the HUBP whose HUBP_VTG_SEL matches this OTG. */
bool RaphaelDcnSetLivePipeBlank(IOMemoryMap *bar5, unsigned otgIndex, bool blank);
/*
 * Opt-in OTG-only probe: blank, V_TOTAL+1 / V_BLANK_START+1, verify, restore
 * GOP totals while still blanked, then unblank. Never leaves +1 as live
 * timing. DP MSA/DIG/PHY are not programmed (not in this slice).
 */
bool RaphaelDcnProbeLiveGopVTotalPlusOne(IOMemoryMap *bar5, unsigned otgIndex);
