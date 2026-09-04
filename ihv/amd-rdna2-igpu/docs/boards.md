# boards.md — Phase R0 board freeze

**Status:** partial — lab evidence recorded; remaining UNKNOWNs before R1.

| Field | Value |
|---|---|
| Host platform | AMD AM5 Hackintosh (motherboard TBD) |
| CPU | Ryzen 9 7950X3D (Raphael iGPU) — assumed from product target; confirm SKU on box |
| iGPU code name | Raphael |
| LLVM `-mcpu` | `gfx1036` |
| PCI VID:DID | `1002:164E` (confirm in IORegistry / System Information) |
| PCI revision | UNKNOWN — measure |
| ACPI / IOKit nub path | UNKNOWN — dump IORegistry on next boot |
| APU connectors in use | UNKNOWN — list HDMI/DP heads that drove the Monterey desktop |
| Discrete GPU present? | **Yes** |
| Discrete GPU | **Nvidia RTX 5080** (Blackwell; no macOS Metal stack) |
| Discrete GPU VID:DID | UNKNOWN — measure (`10de:????`) |
| Discrete connectors | UNKNOWN — list ports on the card |
| WhateverGreen | UNKNOWN version — note if loaded on the Monterey boot |
| `-wegnoegpu` / iGPU disable | **Must not** be required for product |
| Primary display policy | **Connector-driven** |
| Lab OS (evidence) | **macOS Monterey** — iGPU booted **without acceleration** |
| Product OS pin | macOS 26 Tahoe (bring-up target; re-verify on Tahoe) |
| X6000 oracle | Still needed on Tahoe (AMD dGPU machine or add RX 6000 later) |
| Firmware license check | UNKNOWN — list `gc_10_3_6_*`, `dcn_3_1_5_*`, `psp_13_0_5_*` (confirm names) |

## Lab evidence (reporter)

- Booted **Monterey** with the **Raphael iGPU** driving display **without acceleration** (GOP / basic FB path).
- **System Information → Graphics** listed the **iGPU** (recognized) **and** the **RTX 5080**.
- RTX 5080 appearing there is expected PCI/display enumeration without a Metal driver — not evidence of Nvidia acceleration.
- This is a **pre-G1/G2** signal: the iGPU nub is visible to macOS and an unaccelerated desktop on APU outputs is achievable; our kext still must attach for real FB/Metal.

## Acceptance (R0)

- [x] Discrete companion identified (RTX 5080)
- [x] Unaccelerated iGPU boot observed (Monterey)
- [ ] Exact DID/rev + IORegistry path captured under `docs/traces/`
- [ ] `coexistence.md` filled for WEG (if used) + Nvidia companion
- [ ] X6000 IORegistry snapshot on **Tahoe** (ABI oracle — may be second machine)
- [ ] `unknowns.md` updated from this evidence
- [x] Written: NootedRed out of scope; WEG + dGPU coexistence in scope
