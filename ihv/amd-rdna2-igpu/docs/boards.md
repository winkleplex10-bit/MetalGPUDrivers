# boards.md — Phase R0 board freeze (fill in)

**Status:** template — replace UNKNOWN with measured values before R1.

| Field | Value |
|---|---|
| Host platform | AMD AM5 Hackintosh (fill motherboard) |
| CPU | Ryzen 9 7950X3D (or other Raphael SKU) |
| iGPU code name | Raphael |
| LLVM `-mcpu` | `gfx1036` |
| PCI VID:DID | `1002:164E` |
| PCI revision | UNKNOWN — measure |
| ACPI / IOKit nub path | UNKNOWN — measure with iGPU enabled |
| APU connectors in use | UNKNOWN — list HDMI/DP heads on motherboard |
| Discrete GPU present? | **Yes preferred** for coexistence acceptance |
| Discrete GPU VID:DID | UNKNOWN |
| Discrete connectors | UNKNOWN — list ports on the card |
| WhateverGreen | **Required loaded** for dual-GPU acceptance (version UNKNOWN) |
| `-wegnoegpu` / iGPU disable | **Must not** be required for product |
| Primary display policy | **Connector-driven** |
| macOS build | Tahoe (pin exact build) |
| X6000 oracle | On-box dGPU preferred (same Tahoe build) |
| Firmware license check | UNKNOWN — list `gc_10_3_6_*`, `dcn_3_1_5_*`, `psp_13_0_5_*` (confirm names) |

## Acceptance (R0)

- [ ] This file filled (no critical UNKNOWN left for match identity)
- [ ] `coexistence.md` filled for WEG + dGPU baseline
- [ ] X6000 IORegistry snapshot referenced or attached under `docs/traces/`
- [ ] `unknowns.md` started from CURSOR-START-AMD-RDNA2-IGPU §10
- [ ] Written: NootedRed out of scope; WEG + dGPU coexistence in scope
