#pragma once

#include <IOKit/IOMemoryDescriptor.h>
#include <stdint.h>

/*
 * Host-side DMCUB load + GPINT/inbox handshake. Original IOKit C++.
 * Linux living specs (do not paste GPL bodies):
 *   dcn315_resource.c dmcub_support=true
 *   amdgpu_dm.c FIRMWARE_DCN_315_DMUB, AMDGPU_UCODE_ID_DMCUB / PSP load
 *   psp_v13_0.c MODULE_FIRMWARE amdgpu/psp_13_0_5_{toc,ta}.bin
 *   dmub_cmd.h GPINT / inbox command IDs (stable ABI)
 *   dmub DCN31: GPINT via DMCUB_GPINT_DATAIN1, boot status SCRATCH0, response SCRATCH7
 */
struct RaphaelDmubStatus {
	bool firmwarePresent;
	bool pspTocPresent;
	bool pspTaPresent;
	bool dmcubEnabled;
	bool dalFw;
	bool mailboxRdy;
	bool gpintOk;
	bool inboxIssued;
	uint32_t dmcubSize;
	uint32_t headerUcodeVersion;
	uint32_t scratch0;
	uint32_t gpintVersion;
	uint32_t inboxBase;
	uint32_t inboxSize;
	uint32_t inboxRptr;
	uint32_t inboxWptr;
	const char *failReason;
};

void RaphaelDmubLogFirmwarePaths(void);
bool RaphaelDmubLoadAndHandshake(IOMemoryMap *bar5, RaphaelDmubStatus *status);
