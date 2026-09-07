#include <IOKit/IOLib.h>
#include <libkern/libkern.h>
#include <mach/kmod.h>

// Xcode's kernel-extension product type emits <Product>_info.c with kmod_info.
// Keep only start/stop here so Xcode and Makefile do not both define kmod_info.
// Makefile links kext/NvidiaGSP_info.c for the make path.

extern "C" {

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
