//
// Known RTX 3050 / 3060 / 3070 / 3080 / 3090 PCI IDs for logging. Matching is vendor+class, not this list.
// IDs from open-gpu-kernel-modules 610.57.04 README (Ampere GA10x).
//

#ifndef NV_METAL30_PCI_IDS_H
#define NV_METAL30_PCI_IDS_H

#include <libkern/OSTypes.h>

static inline const char *NvidiaChipName(UInt16 deviceId)
{
    switch (deviceId) {
        case 0x2203: return "GeForce RTX 3090 Ti";
        case 0x2204: return "GeForce RTX 3090";
        case 0x2206: return "GeForce RTX 3080";
        case 0x2207: return "GeForce RTX 3070 Ti";
        case 0x2208: return "GeForce RTX 3080 Ti";
        case 0x220A: return "GeForce RTX 3080";
        case 0x2216: return "GeForce RTX 3080";
        case 0x2414: return "GeForce RTX 3060 Ti";
        case 0x2420: return "GeForce RTX 3080 Ti Laptop";
        case 0x2460: return "GeForce RTX 3080 Ti Laptop";
        case 0x2482: return "GeForce RTX 3070 Ti";
        case 0x2484: return "GeForce RTX 3070";
        case 0x2486: return "GeForce RTX 3060 Ti";
        case 0x2487: return "GeForce RTX 3060";
        case 0x2488: return "GeForce RTX 3070";
        case 0x2489: return "GeForce RTX 3060 Ti";
        case 0x249C: return "GeForce RTX 3080 Laptop";
        case 0x249D: return "GeForce RTX 3070 Laptop";
        case 0x24A0: return "GeForce RTX 3060 Laptop";
        case 0x24C7: return "GeForce RTX 3060";
        case 0x24C9: return "GeForce RTX 3060 Ti";
        case 0x24DC: return "GeForce RTX 3080 Laptop";
        case 0x24DD: return "GeForce RTX 3070 Laptop";
        case 0x24E0: return "GeForce RTX 3070 Ti Laptop";
        case 0x2503: return "GeForce RTX 3060";
        case 0x2504: return "GeForce RTX 3060";
        case 0x2507: return "GeForce RTX 3050";
        case 0x2508: return "GeForce RTX 3050 OEM";
        case 0x2520: return "GeForce RTX 3060 Laptop";
        case 0x2521: return "GeForce RTX 3060 Laptop";
        case 0x2523: return "GeForce RTX 3050 Ti Laptop";
        case 0x2544: return "GeForce RTX 3060";
        case 0x2560: return "GeForce RTX 3060 Laptop";
        case 0x2563: return "GeForce RTX 3050 Ti Laptop";
        case 0x2582: return "GeForce RTX 3050";
        case 0x2584: return "GeForce RTX 3050";
        case 0x25A0: return "GeForce RTX 3060 Laptop";
        case 0x25A2: return "GeForce RTX 3050 Laptop";
        case 0x25A5: return "GeForce RTX 3050 Laptop";
        case 0x25A7: return "GeForce RTX 2050";
        case 0x25A9: return "GeForce RTX 2050";
        case 0x25AB: return "GeForce RTX 3050 4GB Laptop";
        case 0x25AC: return "GeForce RTX 3050 6GB Laptop";
        case 0x25AD: return "GeForce RTX 2050";
        case 0x25E0: return "GeForce RTX 3050 Ti Laptop";
        case 0x25E2: return "GeForce RTX 3050 Laptop";
        case 0x25E5: return "GeForce RTX 3050 Laptop";
        case 0x25EC: return "GeForce RTX 3050 6GB Laptop";
        case 0x25ED: return "GeForce RTX 2050";
        case 0x2822: return "GeForce RTX 3050 A Laptop";
        case 0x28A3: return "GeForce RTX 3050 A Laptop";
        case 0x28E3: return "GeForce RTX 3050 A Laptop";
        default:     return "NVIDIA GPU";
    }
}

#endif
