#include <mach/kmod.h>

/* Makefile-only: Xcode generates NvidiaGSP_info.c itself for .kext products. */

#ifdef __cplusplus
extern "C" {
#endif

extern kern_return_t NvidiaGSP_start(kmod_info_t *ki, void *data);
extern kern_return_t NvidiaGSP_stop(kmod_info_t *ki, void *data);

KMOD_EXPLICIT_DECL(dev.metalgpudrivers.NvidiaGSP, "0.1.0", NvidiaGSP_start, NvidiaGSP_stop)

#ifdef __cplusplus
}
#endif
