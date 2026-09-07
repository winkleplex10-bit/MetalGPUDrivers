#include <IOKit/IOLib.h>
#include <libkern/libkern.h>
#include <mach/kmod.h>

extern "C" {

kern_return_t RaphaelFB_start(kmod_info_t *ki, void *d)
{
	(void)d;
	IOLog("RaphaelFB: kmod start %s\n",
	      ki && ki->name ? ki->name : "dev.metalgpudrivers.RaphaelFB");
	return KERN_SUCCESS;
}

kern_return_t RaphaelFB_stop(kmod_info_t *ki, void *d)
{
	(void)ki;
	(void)d;
	IOLog("RaphaelFB: kmod stop\n");
	return KERN_SUCCESS;
}

}
