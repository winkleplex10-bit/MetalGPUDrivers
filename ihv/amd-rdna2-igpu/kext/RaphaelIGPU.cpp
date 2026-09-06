#include <IOKit/IOLib.h>
#include <libkern/libkern.h>
#include <mach/kmod.h>

extern "C" {

kern_return_t RaphaelIGPU_start(kmod_info_t *ki, void *d)
{
	(void)d;
	IOLog("RaphaelIGPU: kmod start %s\n", ki && ki->name ? ki->name : "dev.metalgpudrivers.RaphaelIGPU");
	return KERN_SUCCESS;
}

kern_return_t RaphaelIGPU_stop(kmod_info_t *ki, void *d)
{
	(void)ki;
	(void)d;
	IOLog("RaphaelIGPU: kmod stop\n");
	return KERN_SUCCESS;
}

}
