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
| Connectors in use | UNKNOWN — list HDMI/DP heads actually wired |
| macOS build | Tahoe (pin exact build) |
| Discrete GPU present? | UNKNOWN — note if disabled / primary ownership |
| X6000 oracle machine | UNKNOWN — same Tahoe build, supported Navi dGPU |
| Firmware license check | UNKNOWN — list `gc_10_3_6_*`, `dcn_3_1_5_*`, `psp_13_0_5_*` (confirm names) |

## Acceptance (R0)

- [ ] This file filled (no critical UNKNOWN left for match identity)
- [ ] X6000 IORegistry snapshot referenced or attached under `docs/traces/`
- [ ] `unknowns.md` started from CURSOR-START-AMD-RDNA2-IGPU §10
- [ ] Written: NootedRed is out of scope for product code
