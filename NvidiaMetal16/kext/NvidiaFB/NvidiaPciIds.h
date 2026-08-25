//
// Known GTX 1630 / 1650 / 1660 PCI IDs for logging. Matching is vendor+class, not this list.
// IDs from open-gpu-kernel-modules 610.57.04 README (Turing TU11x).
//

#ifndef NV_METAL16_PCI_IDS_H
#define NV_METAL16_PCI_IDS_H

#include <libkern/OSTypes.h>

static inline const char *NvidiaChipName(UInt16 deviceId)
{
    switch (deviceId) {
        case 0x1F0A: return "GeForce GTX 1650";
        case 0x1F82: return "GeForce GTX 1650";
        case 0x1F83: return "GeForce GTX 1630";
        case 0x1F91: return "GeForce GTX 1650";
        case 0x1F94: return "GeForce GTX 1650";
        case 0x1F95: return "GeForce GTX 1650 Ti";
        case 0x1F96: return "GeForce GTX 1650";
        case 0x1F97: return "GeForce GTX 1650";
        case 0x1F98: return "GeForce GTX 1650";
        case 0x1F99: return "GeForce GTX 1650";
        case 0x1F9C: return "GeForce GTX 1650";
        case 0x1F9D: return "GeForce GTX 1650";
        case 0x1F9F: return "GeForce GTX 1650 Ti";
        case 0x1FDD: return "GeForce GTX 1650";
        case 0x2182: return "GeForce GTX 1660 Ti";
        case 0x2184: return "GeForce GTX 1660";
        case 0x2187: return "GeForce GTX 1650 SUPER";
        case 0x2188: return "GeForce GTX 1650";
        case 0x2189: return "GeForce GTX 1660 SUPER";
        case 0x2191: return "GeForce GTX 1660 Ti";
        case 0x2192: return "GeForce GTX 1650 Ti";
        case 0x21C4: return "GeForce GTX 1660 SUPER";
        case 0x21D1: return "GeForce GTX 1660 Ti";
        default:     return "NVIDIA GPU";
    }
}

#endif
