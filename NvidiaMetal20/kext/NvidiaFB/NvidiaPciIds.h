//
// Known RTX 2060 / 2070 / 2080 PCI IDs for logging. Matching is vendor+class, not this list.
// IDs from open-gpu-kernel-modules 610.57.04 README (Turing TU10x).
//

#ifndef NV_METAL20_PCI_IDS_H
#define NV_METAL20_PCI_IDS_H

#include <libkern/OSTypes.h>

static inline const char *NvidiaChipName(UInt16 deviceId)
{
    switch (deviceId) {
        case 0x1E04: return "GeForce RTX 2080 Ti";
        case 0x1E07: return "GeForce RTX 2080 Ti";
        case 0x1E81: return "GeForce RTX 2080 SUPER";
        case 0x1E82: return "GeForce RTX 2080";
        case 0x1E84: return "GeForce RTX 2070 SUPER";
        case 0x1E87: return "GeForce RTX 2080";
        case 0x1E89: return "GeForce RTX 2060";
        case 0x1E90: return "GeForce RTX 2080";
        case 0x1E91: return "GeForce RTX 2070 Super";
        case 0x1E93: return "GeForce RTX 2080 Super";
        case 0x1EC2: return "GeForce RTX 2070 SUPER";
        case 0x1EC7: return "GeForce RTX 2070 SUPER";
        case 0x1ED0: return "GeForce RTX 2080";
        case 0x1ED1: return "GeForce RTX 2070 Super";
        case 0x1ED3: return "GeForce RTX 2080 Super";
        case 0x1F02: return "GeForce RTX 2070";
        case 0x1F03: return "GeForce RTX 2060";
        case 0x1F06: return "GeForce RTX 2060 SUPER";
        case 0x1F07: return "GeForce RTX 2070";
        case 0x1F08: return "GeForce RTX 2060";
        case 0x1F10: return "GeForce RTX 2070";
        case 0x1F11: return "GeForce RTX 2060";
        case 0x1F12: return "GeForce RTX 2060";
        case 0x1F14: return "GeForce RTX 2070";
        case 0x1F15: return "GeForce RTX 2060";
        case 0x1F42: return "GeForce RTX 2060 SUPER";
        case 0x1F47: return "GeForce RTX 2060 SUPER";
        case 0x1F50: return "GeForce RTX 2070";
        case 0x1F51: return "GeForce RTX 2060";
        case 0x1F54: return "GeForce RTX 2070";
        case 0x1F55: return "GeForce RTX 2060";
        default:     return "NVIDIA GPU";
    }
}

#endif
