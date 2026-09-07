#include "NvidiaController.h"

#include <IOKit/IOLib.h>
#include <libkern/OSByteOrder.h>
#include <pexpert/pexpert.h>

#define super IOService
OSDefineMetaClassAndStructors(NvidiaController, IOService);

bool NvidiaController::init(OSDictionary *dictionary)
{
	if (!super::init(dictionary))
		return false;
	fPci = nullptr;
	fBarCount = 0;
	fDeviceId = 0;
	fMapBars = true;
	for (UInt32 i = 0; i < kNvidiaMaxBars; i++)
		fBars[i] = nullptr;
	return true;
}

bool NvidiaController::claimAllowlisted(IOPCIDevice *pci, UInt16 *outDevice)
{
	OSData *v = OSDynamicCast(OSData, pci->getProperty("vendor-id"));
	OSData *d = OSDynamicCast(OSData, pci->getProperty("device-id"));
	if (!v || !d || v->getLength() < 2 || d->getLength() < 2)
		return false;
	const UInt16 vendor = OSReadLittleInt16(v->getBytesNoCopy(), 0);
	const UInt16 device = OSReadLittleInt16(d->getBytesNoCopy(), 0);
	if (vendor != kNvidiaVendorId)
		return false;
	// Exact DID allow-list only. Never class-match VGA.
	if (device != kNvidiaDeviceId5080)
		return false;
	if (outDevice)
		*outDevice = device;
	return true;
}

IOService *NvidiaController::probe(IOService *provider, SInt32 *score)
{
	(void)score;
	IOPCIDevice *pci = OSDynamicCast(IOPCIDevice, provider);
	if (!pci || !claimAllowlisted(pci, nullptr))
		return nullptr;
	return super::probe(provider, score);
}

bool NvidiaController::mapBars()
{
	const IOItemCount count = fPci->getDeviceMemoryCount();
	fBarCount = 0;
	for (IOItemCount i = 0; i < count && fBarCount < kNvidiaMaxBars; i++) {
		IODeviceMemory *mem = fPci->getDeviceMemoryWithIndex(i);
		if (!mem)
			continue;
		mem->retain();
		fBars[fBarCount++] = mem;
		IOLog("NvidiaController: BAR[%u] phys=0x%llx len=0x%llx (%llu MiB)\n",
		      (unsigned)i,
		      (unsigned long long)mem->getPhysicalAddress(),
		      (unsigned long long)mem->getLength(),
		      (unsigned long long)(mem->getLength() / (1024ull * 1024ull)));
	}
	return fBarCount > 0;
}

void NvidiaController::publishIdentity(UInt16 deviceId)
{
	setName("NvidiaController");
	setProperty("vendor-id", (UInt64)kNvidiaVendorId, 32);
	setProperty("device-id", (UInt64)deviceId, 32);
	setProperty("model", kNvidiaModelName5080);
	setProperty("NvidiaArch", kNvidiaArchName);
	setProperty("NvidiaPhase", kNvidiaPhaseEnumerate);
	setProperty("NvidiaGsp", false);
	setProperty("NvidiaMetal", false);
	setProperty("NvidiaBarCount", (UInt64)fBarCount, 32);

	OSArray *barLens = OSArray::withCapacity(fBarCount);
	for (UInt32 i = 0; i < fBarCount; i++) {
		OSNumber *n = OSNumber::withNumber((unsigned long long)fBars[i]->getLength(), 64);
		if (barLens && n)
			barLens->setObject(n);
		if (n)
			n->release();
	}
	if (barLens) {
		setProperty("NvidiaBarLengths", barLens);
		barLens->release();
	}
}

bool NvidiaController::start(IOService *provider)
{
	int dummy = 0;
	if (PE_parse_boot_argn("-nvoff", &dummy, sizeof(dummy))) {
		IOLog("NvidiaController: disabled by -nvoff\n");
		return false;
	}

	fPci = OSDynamicCast(IOPCIDevice, provider);
	if (!fPci || !super::start(provider))
		return false;
	if (!claimAllowlisted(fPci, &fDeviceId))
		return false;

	int map = 1;
	if (PE_parse_boot_argn("nv_map", &map, sizeof(map)))
		fMapBars = map != 0;

	// Memory space only. Do not enable bus-master yet — no DMA/GSP in N1-enumerate.
	fPci->setMemoryEnable(true);

	if (fMapBars) {
		if (!mapBars()) {
			IOLog("NvidiaController: no PCI BARs mapped for %04x:%04x\n",
			      kNvidiaVendorId, fDeviceId);
			return false;
		}
	} else {
		IOLog("NvidiaController: nv_map=0 — claim without BAR maps\n");
	}

	publishIdentity(fDeviceId);
	registerService();
	IOLog("NvidiaController: attached %04x:%04x (%s) bars=%u phase=%s\n",
	      kNvidiaVendorId, fDeviceId, kNvidiaArchName, (unsigned)fBarCount,
	      kNvidiaPhaseEnumerate);
	return true;
}

void NvidiaController::stop(IOService *provider)
{
	for (UInt32 i = 0; i < fBarCount; i++) {
		if (fBars[i]) {
			fBars[i]->release();
			fBars[i] = nullptr;
		}
	}
	fBarCount = 0;
	fPci = nullptr;
	super::stop(provider);
}
