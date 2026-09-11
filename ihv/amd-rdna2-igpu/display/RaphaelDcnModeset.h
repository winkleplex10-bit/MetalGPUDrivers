#pragma once

#include <IOKit/IOMemoryDescriptor.h>
#include <stdint.h>

/*
 * First hardware modeset slice: reaffirm the live GOP 3840×2160 pipe on OTG0.
 * DCN writes only after DMUB handshake. Do not blank; do not invent PHY/PLL
 * offsets missing from dcn_3_1_5_offset.h.
 */
bool RaphaelDcnReaffirmLiveGop4k(IOMemoryMap *bar5, unsigned otgIndex);
