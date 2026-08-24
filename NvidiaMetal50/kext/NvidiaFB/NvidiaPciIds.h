//
// Known NVIDIA PCI device IDs for logging. Matching is vendor+class, not this list.
// GB20x IDs from open-gpu-kernel-modules 610.57.04 README.
//

#ifndef NV_METAL50_PCI_IDS_H
#define NV_METAL50_PCI_IDS_H

#include <libkern/OSTypes.h>

static inline const char *NvidiaChipName(UInt16 deviceId)
{
    switch (deviceId) {
        case 0x2B85: return "GeForce RTX 5090";
        case 0x2B87: return "GeForce RTX 5090 D";
        case 0x2B8C: return "GeForce RTX 5090 D v2";
        case 0x2BB1: return "RTX PRO 6000 Blackwell";
        case 0x2BB4: return "RTX PRO 6000 Blackwell";
        case 0x2BB5: return "RTX PRO 6000 Blackwell";
        case 0x2C02: return "GeForce RTX 5080";
        case 0x2C05: return "GeForce RTX 5070 Ti";
        case 0x2C18: return "GeForce RTX 5090 Laptop";
        case 0x2C19: return "GeForce RTX 5080 Laptop";
        case 0x2C58: return "GeForce RTX 5090 Laptop";
        case 0x2C59: return "GeForce RTX 5080 Laptop";
        case 0x2D04: return "GeForce RTX 5060 Ti";
        case 0x2D05: return "GeForce RTX 5060";
        case 0x2D18: return "GeForce RTX 5070 Laptop";
        case 0x2D19: return "GeForce RTX 5060 Laptop";
        case 0x2D83: return "GeForce RTX 5050";
        case 0x2F04: return "GeForce RTX 5070";
        case 0x2F06: return "GeForce RTX 5060";
        case 0x2F18: return "GeForce RTX 5070 Ti Laptop";
        default:     return "NVIDIA GPU";
    }
}

#endif
