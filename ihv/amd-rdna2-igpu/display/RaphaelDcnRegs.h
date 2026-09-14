#pragma once

#include <IOKit/IOMemoryDescriptor.h>
#include <libkern/OSByteOrder.h>
#include <stdint.h>

/*
 * Cited DCN 3.1.5 MMIO (dword offsets). Do not invent names missing here.
 *   dcn_3_1_5_offset.h / dcn_3_1_5_sh_mask.h
 *   hw_factory_dcn315.c DCN_BASE__INST0_SEG2 = 0x34C0
 * https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/amd/include/asic_reg/dcn/dcn_3_1_5_offset.h
 * https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/amd/include/asic_reg/dcn/dcn_3_1_5_sh_mask.h
 * https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/amd/display/dc/gpio/dcn315/hw_factory_dcn315.c
 */
static const uint32_t kRaphaelDcnSeg2 = 0x34C0;
static const uint32_t kRaphaelHpdIntStatusReg[] = { 0x1f14, 0x1f1c, 0x1f24, 0x1f2c, 0x1f34 };
static const uint32_t kRaphaelDcGpioHpdY = 0x28f7;
static const uint32_t kRaphaelHpdIntStatusMask = 0x00000001;
static const uint32_t kRaphaelHpdSenseMask = 0x00000002;
static const uint32_t kRaphaelHpdSenseDelayedMask = 0x00000010;
static const uint32_t kRaphaelHpdRxIntMask = 0x00000100;
static const uint32_t kRaphaelGpioHpdYMask[] = { 0x00000001, 0x00000100, 0x00010000,
						  0x01000000, 0x04000000, 0x10000000 };
static const unsigned kRaphaelOtgCount = 4;
static const uint32_t kRaphaelOtgHTotalReg[] = { 0x1b2a, 0x1baa, 0x1c2a, 0x1caa };
static const uint32_t kRaphaelOtgVTotalReg[] = { 0x1b2f, 0x1baf, 0x1c2f, 0x1caf };
static const uint32_t kRaphaelOtgControlReg[] = { 0x1b41, 0x1bc1, 0x1c41, 0x1cc1 };
static const uint32_t kRaphaelOtgHBlankReg[] = { 0x1b2b, 0x1bab, 0x1c2b, 0x1cab };
static const uint32_t kRaphaelOtgVBlankReg[] = { 0x1b36, 0x1bb6, 0x1c36, 0x1cb6 };
static const uint32_t kRaphaelOdmOptcClkReg[] = { 0x1acf, 0x1adf, 0x1aef, 0x1aff };
static const uint32_t kRaphaelOtgTotalMask = 0x00007FFF;
static const uint32_t kRaphaelOtgMasterEnMask = 0x00000001;
static const uint32_t kRaphaelOtgCurMasterEnMask = 0x00010000;
static const uint32_t kRaphaelOtgBlankStartMask = 0x00007FFF;
static const uint32_t kRaphaelOtgBlankEndMask = 0x7FFF0000;
static const uint32_t kRaphaelOtgBlankEndShift = 16;
static const uint32_t kRaphaelOptcClkEnMask = 0x00000002;
static const uint32_t kRaphaelOptcClkOnMask = 0x00000004;

/*
 * OTG_BLANK_CONTROL / OTG_BLANK_DATA_EN are not in dcn_3_1_5_offset.h
 * (OTG_CONTROL 0x1b41 then INTERLACE 0x1b44; do not invent 0x1b42).
 * Closest cited pipe blank (not timing, not MASTER_EN, not PHY/PLL):
 * HUBP DCHUBP_CNTL.HUBP_BLANK_EN.
 */
static const uint32_t kRaphaelHubpCntlReg[] = { 0x05f3, 0x06cf, 0x07ab, 0x0887 };
static const uint32_t kRaphaelHubpBlankEnMask = 0x00000001;
static const uint32_t kRaphaelHubpInBlankMask = 0x00000008;
static const uint32_t kRaphaelHubpVtgSelMask = 0x000000F0;
static const uint32_t kRaphaelHubpVtgSelShift = 4;
static const unsigned kRaphaelHubpCount = 4;

/* DMCUB — dcn_3_1_5_offset.h BASE_IDX 2 (same SEG2 as HPD/OTG). */
static const uint32_t kRaphaelDmcubInbox1Base = 0x01d4;
static const uint32_t kRaphaelDmcubInbox1Size = 0x01d5;
static const uint32_t kRaphaelDmcubInbox1Wptr = 0x01d6;
static const uint32_t kRaphaelDmcubInbox1Rptr = 0x01d7;
static const uint32_t kRaphaelDmcubScratch0 = 0x01e3;
static const uint32_t kRaphaelDmcubScratch7 = 0x01ea;
static const uint32_t kRaphaelDmcubCntl = 0x01f6;
static const uint32_t kRaphaelDmcubGpintDatain1 = 0x01f8;
static const uint32_t kRaphaelDmcubEnableMask = 0x00010000; /* DMCUB_CNTL__DMCUB_ENABLE */

static const IOByteCount kRaphaelBar5Expected = 512 * 1024;
static const IOByteCount kRaphaelBar2Bytes = 2 * 1024 * 1024;

static inline bool raphaelDcnRead32(IOMemoryMap *map, uint32_t dwordOff, uint32_t *out)
{
	if (!map || !out)
		return false;
	const IOByteCount byteOff = (IOByteCount)dwordOff * 4;
	if (byteOff + 4 > map->getLength())
		return false;
	const uint8_t *base = reinterpret_cast<const uint8_t *>(map->getVirtualAddress());
	if (!base)
		return false;
	*out = OSReadLittleInt32(base, byteOff);
	return true;
}

static inline bool raphaelDcnWrite32(IOMemoryMap *map, uint32_t dwordOff, uint32_t value)
{
	if (!map)
		return false;
	const IOByteCount byteOff = (IOByteCount)dwordOff * 4;
	if (byteOff + 4 > map->getLength())
		return false;
	uint8_t *base = reinterpret_cast<uint8_t *>(map->getVirtualAddress());
	if (!base)
		return false;
	OSWriteLittleInt32(base, byteOff, value);
	return true;
}
