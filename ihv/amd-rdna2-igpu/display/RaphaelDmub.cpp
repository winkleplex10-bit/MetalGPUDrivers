#include "RaphaelDmub.h"
#include "RaphaelDcnRegs.h"

#include <IOKit/IODeviceMemory.h>
#include <IOKit/IOLib.h>
#include <libkern/OSByteOrder.h>
#include <libkern/libkern.h>
#include <string.h>
#include <sys/proc.h>
#include <sys/vnode.h>

/*
 * Public firmware container fields (linux amdgpu_ucode.h common_firmware_header).
 * Used only to log size/version. Do not disassemble the payload.
 */
static const uint32_t kRaphaelFwUcodeVersionOff = 24;
static const uint32_t kRaphaelDmcubInstConstOff = 40;
static const uint32_t kRaphaelMaxFwBytes = 8 * 1024 * 1024;

/* dmub_cmd.h — GPINT / inbox IDs (stable ABI). */
static const uint32_t kRaphaelDmubGpintGetFwVersion = 1;
static const uint32_t kRaphaelDmubGpintNotifyStreamMask = 8;
static const uint32_t kRaphaelDmubCmdQueryFeatureCaps = 6;
static const uint32_t kRaphaelDmubRbCmdSize = 64;
static const uint32_t kRaphaelFwBootDal = (1u << 0);
static const uint32_t kRaphaelFwBootMailboxRdy = (1u << 1);

static const char *const kRaphaelFwDirs[] = {
	"/Library/Extensions/RaphaelIGPU.kext/Contents/Resources",
	"/System/Library/Extensions/RaphaelIGPU.kext/Contents/Resources",
};

static const char kRaphaelDmcubName[] = "dcn_3_1_5_dmcub.bin";
static const char kRaphaelPspTocName[] = "psp_13_0_5_toc.bin";
static const char kRaphaelPspTaName[] = "psp_13_0_5_ta.bin";

static bool raphaelReadFile(const char *path, OSData **out)
{
	if (!path || !out)
		return false;
	*out = nullptr;

	vfs_context_t ctx = vfs_context_create(nullptr);
	if (!ctx)
		return false;

	vnode_t vp = NULLVP;
	int err = vnode_lookup(path, 0, &vp, ctx);
	if (err != 0 || vp == NULLVP) {
		vfs_context_rele(ctx);
		return false;
	}

	off_t fileSize = 0;
	vnode_attr va;
	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_data_size);
	err = vnode_getattr(vp, &va, ctx);
	if (err == 0)
		fileSize = (off_t)va.va_data_size;
	if (err != 0 || fileSize <= 0 || fileSize > (off_t)kRaphaelMaxFwBytes) {
		vnode_put(vp);
		vfs_context_rele(ctx);
		return false;
	}

	void *buf = IOMalloc((size_t)fileSize);
	if (!buf) {
		vnode_put(vp);
		vfs_context_rele(ctx);
		return false;
	}

	int resid = 0;
	err = vn_rdwr(UIO_READ, vp, (caddr_t)buf, (int)fileSize, 0, UIO_SYSSPACE, 0,
		      vfs_context_ucred(ctx), &resid, vfs_context_proc(ctx));
	vnode_put(vp);
	vfs_context_rele(ctx);
	if (err != 0 || resid != 0) {
		IOFree(buf, (size_t)fileSize);
		return false;
	}

	*out = OSData::withBytes(buf, (unsigned int)fileSize);
	IOFree(buf, (size_t)fileSize);
	return *out != nullptr;
}

static bool raphaelLoadNamedFw(const char *name, OSData **out, char *usedPath, size_t usedPathLen)
{
	if (usedPath && usedPathLen)
		usedPath[0] = 0;
	for (unsigned i = 0; i < sizeof(kRaphaelFwDirs) / sizeof(kRaphaelFwDirs[0]); i++) {
		char path[256];
		snprintf(path, sizeof(path), "%s/%s", kRaphaelFwDirs[i], name);
		if (raphaelReadFile(path, out)) {
			if (usedPath && usedPathLen)
				strlcpy(usedPath, path, usedPathLen);
			return true;
		}
	}
	return false;
}

static uint32_t raphaelGpintWord(uint32_t command, uint32_t param, uint32_t status)
{
	return (param & 0xFFFFu) | ((command & 0xFFFu) << 16) | ((status & 0xFu) << 28);
}

static bool raphaelGpintCommand(IOMemoryMap *bar5, uint32_t command, uint32_t param,
				uint32_t *response)
{
	const uint32_t pending = raphaelGpintWord(command, param, 1);
	const uint32_t acked = raphaelGpintWord(command, param, 0);
	if (!raphaelDcnWrite32(bar5, kRaphaelDcnSeg2 + kRaphaelDmcubGpintDatain1, pending))
		return false;

	for (int i = 0; i < 100; i++) {
		uint32_t seen = 0;
		if (raphaelDcnRead32(bar5, kRaphaelDcnSeg2 + kRaphaelDmcubGpintDatain1, &seen) &&
		    seen == acked) {
			if (response)
				raphaelDcnRead32(bar5, kRaphaelDcnSeg2 + kRaphaelDmcubScratch7,
						 response);
			return true;
		}
		IOSleep(1);
	}
	return false;
}

static bool raphaelInboxQueryCaps(IOMemoryMap *bar5, RaphaelDmubStatus *st)
{
	if (!st->mailboxRdy || st->inboxSize < kRaphaelDmubRbCmdSize || st->inboxBase == 0)
		return false;
	if (st->inboxWptr + kRaphaelDmubRbCmdSize > st->inboxSize)
		return false;

	IODeviceMemory *mem =
		IODeviceMemory::withRange((IOPhysicalAddress)st->inboxBase, st->inboxSize);
	if (!mem)
		return false;
	IOMemoryMap *map = mem->map();
	mem->release();
	if (!map) {
		IOLog("RaphaelDmub: inbox GPU addr=0x%x size=%u map failed (no BAR0 map)\n",
		      st->inboxBase, st->inboxSize);
		return false;
	}

	uint8_t *ring = reinterpret_cast<uint8_t *>(map->getVirtualAddress());
	if (!ring || map->getLength() < st->inboxWptr + kRaphaelDmubRbCmdSize) {
		map->release();
		return false;
	}

	uint8_t cmd[64];
	bzero(cmd, sizeof(cmd));
	/* dmub_cmd_header: type=QUERY_FEATURE_CAPS (6), payload_bytes=0 */
	cmd[0] = (uint8_t)kRaphaelDmubCmdQueryFeatureCaps;
	memcpy(ring + st->inboxWptr, cmd, kRaphaelDmubRbCmdSize);
	const uint32_t newWptr = st->inboxWptr + kRaphaelDmubRbCmdSize;
	const bool wrote = raphaelDcnWrite32(bar5, kRaphaelDcnSeg2 + kRaphaelDmcubInbox1Wptr, newWptr);
	map->release();
	if (!wrote)
		return false;

	for (int i = 0; i < 100; i++) {
		uint32_t rptr = 0;
		if (raphaelDcnRead32(bar5, kRaphaelDcnSeg2 + kRaphaelDmcubInbox1Rptr, &rptr) &&
		    rptr == newWptr) {
			st->inboxWptr = newWptr;
			return true;
		}
		IOSleep(1);
	}
	IOLog("RaphaelDmub: inbox wptr bump timeout\n");
	return false;
}

void RaphaelDmubLogFirmwarePaths(void)
{
	IOLog("RaphaelDmub: firmware Resources dcn_3_1_5_dmcub.bin + psp_13_0_5_{toc,ta}.bin "
	      "LICENSE.amdgpu (linux-firmware; no reverse/decompile)\n");
}

bool RaphaelDmubLoadAndHandshake(IOMemoryMap *bar5, RaphaelDmubStatus *status)
{
	if (!status)
		return false;
	bzero(status, sizeof(*status));
	status->failReason = "uninitialized";

	RaphaelDmubLogFirmwarePaths();

	OSData *dmcub = nullptr;
	OSData *toc = nullptr;
	OSData *ta = nullptr;
	char dmcubPath[256];
	char tocPath[256];
	char taPath[256];
	status->firmwarePresent = raphaelLoadNamedFw(kRaphaelDmcubName, &dmcub, dmcubPath,
						     sizeof(dmcubPath));
	status->pspTocPresent = raphaelLoadNamedFw(kRaphaelPspTocName, &toc, tocPath, sizeof(tocPath));
	status->pspTaPresent = raphaelLoadNamedFw(kRaphaelPspTaName, &ta, taPath, sizeof(taPath));

	if (status->firmwarePresent && dmcub) {
		status->dmcubSize = dmcub->getLength();
		const uint8_t *bytes = (const uint8_t *)dmcub->getBytesNoCopy();
		if (bytes && status->dmcubSize >= kRaphaelDmcubInstConstOff + 4)
			status->headerUcodeVersion =
				OSReadLittleInt32(bytes, kRaphaelFwUcodeVersionOff);
		IOLog("RaphaelDmub: loaded %s bytes=%u ucode_version=0x%08x (container only; "
		      "unmodified)\n",
		      dmcubPath, status->dmcubSize, status->headerUcodeVersion);
	} else {
		IOLog("RaphaelDmub: %s missing (fetch with make fetch-firmware; install in kext "
		      "Resources)\n",
		      kRaphaelDmcubName);
		status->failReason = "dcn_3_1_5_dmcub.bin missing from kext Resources";
		if (toc)
			toc->release();
		if (ta)
			ta->release();
		return false;
	}
	if (status->pspTocPresent)
		IOLog("RaphaelDmub: PSP toc %s bytes=%u (psp_v13_0.c Raphael MP0 13.0.5)\n", tocPath,
		      toc ? toc->getLength() : 0);
	else
		IOLog("RaphaelDmub: psp_13_0_5_toc.bin missing\n");
	if (status->pspTaPresent)
		IOLog("RaphaelDmub: PSP ta %s bytes=%u\n", taPath, ta ? ta->getLength() : 0);
	else
		IOLog("RaphaelDmub: psp_13_0_5_ta.bin missing\n");

	if (dmcub)
		dmcub->release();
	if (toc)
		toc->release();
	if (ta)
		ta->release();

	if (!bar5) {
		status->failReason = "BAR5 DCN MMIO not mapped";
		return false;
	}

	uint32_t cntl = 0;
	if (!raphaelDcnRead32(bar5, kRaphaelDcnSeg2 + kRaphaelDmcubCntl, &cntl)) {
		status->failReason = "DMCUB_CNTL unread";
		return false;
	}
	status->dmcubEnabled = (cntl & kRaphaelDmcubEnableMask) != 0;
	if (!raphaelDcnRead32(bar5, kRaphaelDcnSeg2 + kRaphaelDmcubScratch0, &status->scratch0)) {
		status->failReason = "DMCUB_SCRATCH0 unread";
		return false;
	}
	status->dalFw = (status->scratch0 & kRaphaelFwBootDal) != 0;
	status->mailboxRdy = (status->scratch0 & kRaphaelFwBootMailboxRdy) != 0;
	raphaelDcnRead32(bar5, kRaphaelDcnSeg2 + kRaphaelDmcubInbox1Base, &status->inboxBase);
	raphaelDcnRead32(bar5, kRaphaelDcnSeg2 + kRaphaelDmcubInbox1Size, &status->inboxSize);
	raphaelDcnRead32(bar5, kRaphaelDcnSeg2 + kRaphaelDmcubInbox1Rptr, &status->inboxRptr);
	raphaelDcnRead32(bar5, kRaphaelDcnSeg2 + kRaphaelDmcubInbox1Wptr, &status->inboxWptr);

	IOLog("RaphaelDmub: DMCUB_CNTL raw=0x%08x ENABLE=%u SCRATCH0=0x%08x dal_fw=%u mailbox_rdy=%u "
	      "INBOX1 base=0x%x size=%u rptr=%u wptr=%u\n",
	      cntl, status->dmcubEnabled ? 1 : 0, status->scratch0, status->dalFw ? 1 : 0,
	      status->mailboxRdy ? 1 : 0, status->inboxBase, status->inboxSize, status->inboxRptr,
	      status->inboxWptr);

	/*
	 * Lab 0.2.6: ENABLE=1 mailbox_rdy=1 dal_fw=0 (SCRATCH0=0x42). That is
	 * GOP/VBIOS DMCUB, not "DMCUB dead". Linux DAL sets
	 * DMCUB_FW_BOOT_STATUS_BIT_DAL_FW after driver load; GOP does not.
	 * GPINT on BAR5 (DMCUB_GPINT_DATAIN1). PSP/MP0 only if ENABLE or
	 * mailbox_rdy is actually off.
	 */
	if (!status->dmcubEnabled || !status->mailboxRdy) {
		status->failReason = "PSP authenticate required; MP0 C2PMSG not in dcn_3_1_5_offset.h";
		IOLog("RaphaelDmub: DMCUB not running (ENABLE=%u mailbox_rdy=%u dal_fw=%u). "
		      "Linux cold-loads via PSP 13.0.5; cannot program MP0 from BAR5. "
		      "Keep GOP wrap.\n",
		      status->dmcubEnabled ? 1 : 0, status->mailboxRdy ? 1 : 0,
		      status->dalFw ? 1 : 0);
		return false;
	}
	IOLog("RaphaelDmub: DMCUB running ENABLE=1 mailbox_rdy=1 dal_fw=%u — GPINT "
	      "(dal_fw=0 is GOP/VBIOS, not a PSP abort)\n",
	      status->dalFw ? 1 : 0);

	if (!raphaelGpintCommand(bar5, kRaphaelDmubGpintGetFwVersion, 0, &status->gpintVersion)) {
		status->failReason = "GPINT GET_FW_VERSION timeout";
		IOLog("RaphaelDmub: GPINT GET_FW_VERSION timeout (DMCUB_GPINT_DATAIN1)\n");
		return false;
	}
	status->gpintOk = true;
	IOLog("RaphaelDmub: GPINT GET_FW_VERSION ok response=0x%08x\n", status->gpintVersion);

	/* Notify live stream 0 (OTG0). dmub_cmd.h DMUB_GPINT__IDLE_OPT_NOTIFY_STREAM_MASK. */
	uint32_t maskResp = 0;
	if (!raphaelGpintCommand(bar5, kRaphaelDmubGpintNotifyStreamMask, 1u, &maskResp))
		IOLog("RaphaelDmub: GPINT NOTIFY_STREAM_MASK timeout (non-fatal if GET_FW ok)\n");
	else
		IOLog("RaphaelDmub: GPINT NOTIFY_STREAM_MASK stream0 ok resp=0x%08x\n", maskResp);

	status->inboxIssued = raphaelInboxQueryCaps(bar5, status);
	if (status->inboxIssued)
		IOLog("RaphaelDmub: inbox QUERY_FEATURE_CAPS issued (DMUB_CMD type 6, 64-byte RB)\n");
	else
		IOLog("RaphaelDmub: inbox not issued (mailbox_rdy=%u base=0x%x); GPINT handshake "
		      "holds\n",
		      status->mailboxRdy ? 1 : 0, status->inboxBase);

	status->failReason = nullptr;
	return true;
}
