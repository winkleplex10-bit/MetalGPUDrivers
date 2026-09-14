#include <IOKit/IOLib.h>
#include <libkern/libkern.h>
#include <mach/kmod.h>

extern "C" {

kern_return_t RaphaelIGPU_start(kmod_info_t *ki, void *d);
kern_return_t RaphaelIGPU_stop(kmod_info_t *ki, void *d);

KMOD_EXPLICIT_DECL(dev_metalgpudrivers_RaphaelIGPU, "0.2.10", RaphaelIGPU_start, RaphaelIGPU_stop)

kern_return_t RaphaelIGPU_start(kmod_info_t *ki, void *d)
{
	(void)d;
	IOLog("RaphaelIGPU: kmod start %s\n",
	      (ki != nullptr) ? ki->name : "dev.metalgpudrivers.RaphaelIGPU");
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
