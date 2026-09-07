#include <IOKit/IOLib.h>
#include <libkern/libkern.h>
#include <mach/kmod.h>

extern "C" {

kern_return_t NvidiaGSP_start(kmod_info_t *ki, void *d);
kern_return_t NvidiaGSP_stop(kmod_info_t *ki, void *d);

KMOD_EXPLICIT_DECL(dev.metalgpudrivers.NvidiaGSP, "0.1.0", NvidiaGSP_start, NvidiaGSP_stop)

kern_return_t NvidiaGSP_start(kmod_info_t *ki, void *d)
{
	(void)d;
	IOLog("NvidiaGSP: kmod start %s\n",
	      (ki != nullptr) ? ki->name : "dev.metalgpudrivers.NvidiaGSP");
	return KERN_SUCCESS;
}

kern_return_t NvidiaGSP_stop(kmod_info_t *ki, void *d)
{
	(void)ki;
	(void)d;
	IOLog("NvidiaGSP: kmod stop\n");
	return KERN_SUCCESS;
}

}
