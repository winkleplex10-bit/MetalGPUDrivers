//
// Known RTX 4050 / 4060 / 4070 / 4080 / 4090 PCI IDs for logging. Matching is vendor+class, not this list.
// IDs from open-gpu-kernel-modules 610.57.04 README (Ada Lovelace AD10x).
//

#ifndef NV_METAL40_PCI_IDS_H
#define NV_METAL40_PCI_IDS_H

#include <libkern/OSTypes.h>

static inline const char *NvidiaChipName(UInt16 deviceId)
{
    switch (deviceId) {
        case 0x2684: return "GeForce RTX 4090";
        case 0x2685: return "GeForce RTX 4090 D";
        case 0x2689: return "GeForce RTX 4070 Ti SUPER";
        case 0x2702: return "GeForce RTX 4080 SUPER";
        case 0x2704: return "GeForce RTX 4080";
        case 0x2705: return "GeForce RTX 4070 Ti SUPER";
        case 0x2709: return "GeForce RTX 4070";
        case 0x2717: return "GeForce RTX 4090 Laptop";
        case 0x2757: return "GeForce RTX 4090 Laptop";
        case 0x2782: return "GeForce RTX 4070 Ti";
        case 0x2783: return "GeForce RTX 4070 SUPER";
        case 0x2786: return "GeForce RTX 4070";
        case 0x2788: return "GeForce RTX 4060 Ti";
        case 0x27A0: return "GeForce RTX 4080 Laptop";
        case 0x27E0: return "GeForce RTX 4080 Laptop";
        case 0x2803: return "GeForce RTX 4060 Ti";
        case 0x2805: return "GeForce RTX 4060 Ti";
        case 0x2808: return "GeForce RTX 4060";
        case 0x2820: return "GeForce RTX 4070 Laptop";
        case 0x2860: return "GeForce RTX 4070 Laptop";
        case 0x2882: return "GeForce RTX 4060";
        case 0x28A0: return "GeForce RTX 4060 Laptop";
        case 0x28A1: return "GeForce RTX 4050 Laptop";
        case 0x28E0: return "GeForce RTX 4060 Laptop";
        case 0x28E1: return "GeForce RTX 4050 Laptop";
        default:     return "NVIDIA GPU";
    }
}

#endif
