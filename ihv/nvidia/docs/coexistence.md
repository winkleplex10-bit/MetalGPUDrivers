# Nvidia / Raphael coexistence (same Hackintosh)

Lab host: MacPro7,1 SMBIOS, Sequoia 15.7.x, dual GPU:

| GPU | VID:DID | Nub | Our kext | Match category |
|---|---|---|---|---|
| AMD Raphael iGPU | `1002:164E` | `IGPU@0` | `RaphaelIGPU` → `RaphaelController` | `RaphaelHW` |
| Nvidia RTX 5080 | `10de:2C02` | `GFX0@0` | `NvidiaGSP` → `NvidiaController` | `NvidiaHW` |

## Rules

1. **Exact DID match only** on both sides — never VGA class-wide `0x03000000`.
2. Separate `IOMatchCategory` values so neither steals the other's `IOService` client slot; Apple `IONDRVFramebuffer` may still own `IOFramebuffer` on the 5080 until Phase N2.
3. Display ownership today: Raphael / iGPU path owns the main desktop; 5080 is PCI-visible with `IONDRVFramebuffer` child and **No Kext Loaded** for a real Nvidia stack.
4. Do not WhateverGreen-spoof Nvidia IDs onto AMD personalities (or vice versa).
5. GSP bring-up on the 5080 must not assume it is the boot console; Phase N2 must decide console vs secondary carefully.
