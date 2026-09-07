# boards.md — Phase R0 board freeze

**Status:** measured from Sequoia dump `docs/traces/sequoia-7950x3d/` (2026-09-06) plus OpenCore log + SysReport (2026-09-07). `RaphaelIGPU.kext` v0.1.0 **did not load** on that boot (`IOGraphicsFamily` missing from Boot KC). Controller-only kext is in tree; on-box attach still unproven. Remaining gaps listed at bottom. Living plan: [`docs/CURSOR-START-AMD-RDNA2-IGPU.md`](../../../docs/CURSOR-START-AMD-RDNA2-IGPU.md) §0.

| Field | Value |
|---|---|
| Host platform | AMD AM5 Hackintosh (motherboard ASUS by subsystem `1043:8877`) |
| CPU | **Ryzen 9 7950X3D** (Raphael iGPU) |
| SMBIOS | **MacPro7,1** |
| iGPU code name | Raphael |
| LLVM `-mcpu` | `gfx1036` |
| PCI VID:DID | **`1002:164E`** |
| PCI revision | **`0xC9`** |
| PCI BDF | **`12:0:0`** (`pcidebug`) |
| Subsystem | **`1043:8877`** (ASUS) |
| IOKit nub name | **`IGPU@0`** (`IOPCIDevice`) |
| ACPI path | **`_SB.PCI0.GP17.VGA`** (`IOACPIPlane:/_SB/PCI0@0/GP17@80001/VGA@0`) |
| `compatible` | `pci1043,8877`, `pci1002,164e`, `pciclass,030000`, `VGA`, `IGPU` |
| Parent bridge | `GP17@8,1` under `PCI0` |
| Current FB | **`IONDRVFramebuffer`** (`.display_boot`) — unaccelerated NDRV/GOP path |
| Also attached | **`AMDSupport`** (Apple, probe 65050, vendor-wide AMD VGA match) — not a Metal stack |
| APU display | **Main display** 3840×2160@HiDPI; System Information: VRAM 31 MB, **No Kext Loaded** (acceleration) |
| Discrete GPU | **Nvidia RTX 5080** `10de:2c02` rev `0xA1`, subsystem `1462:5315` (MSI), nub **`GFX0@0`**, BDF `1:0:0`, Slot-1 — also filed under [`ihv/nvidia/docs/boards.md`](../../nvidia/docs/boards.md) |
| dGPU ACPI | `_SB.PCI0.GPP0.VGA` |
| Lilu | **1.7.2** (`as.vit9696.Lilu`) |
| WhateverGreen | **1.7.1d7** (`as.vit9696.laobamac.WhateverGreen` — laobamac fork) |
| `-wegnoegpu` / iGPU disable | Not indicated; iGPU is main display |
| Primary display policy | **Connector-driven** (product); currently APU owns main display |
| Lab OS (this dump) | **macOS Sequoia 15.7.8 (24G824)** |
| Product OS pin | macOS 26 Tahoe (re-verify; Sequoia is valid bring-up OS) |
| X6000 oracle | **Still needed** — 5080 cannot provide AMD Metal ABI |
| Firmware license | UNKNOWN — `gc_10_3_6_*` / `dcn_3_1_5_*` / `psp_13_0_5_*` (confirm names) |
| First kext | `dev.metalgpudrivers.RaphaelIGPU` — enumerate + ATOM; GOP wrap is `RaphaelFB.kext` (needs IOGraphicsFamily in the same KC). See [BUILD.md](BUILD.md) |

## Match personality (from dump)

```
IOPCIPrimaryMatch  = 0x164E1002
IOPCIRevisionMatch = 0xC9??????   (optional; rev C9 observed)
IONameMatch        = display      (optional; both GPUs use IOName display — prefer PCI match only)
```

**Do not** match `IOPCIClassMatch` `0x03000000` alone — that would also catch the RTX 5080’s VGA class and other AMD cards. **DID-only** match on `0x164E`.

## Lab evidence

- Sequoia dump proves dual-GPU enumeration: Raphael iGPU + RTX 5080.
- iGPU already drives a real 4K desktop via **IONDRVFramebuffer** (G2 baseline exists).
- Apple **AMDSupport** attaches to the iGPU today; our kext must coexist or displace only our FB path carefully — do not fight AMDSupport globally.
- No `AMDRadeonX6000*` / Metal plugin on either GPU.

## Acceptance (R0)

- [x] Exact DID/rev + IORegistry path captured
- [x] Discrete companion identified (RTX 5080)
- [x] Unaccelerated iGPU desktop observed (Sequoia + prior Monterey)
- [x] WEG/Lilu versions recorded
- [ ] Physical APU port label (HDMI vs DP which motherboard connector) — VFCT has both HDMI-A and DP
- [ ] On-box `RaphaelController` attach after Boot KC inject (7 Sep 2026 inject failed before match)
- [ ] OpenCore DeviceProperties snippet for IGPU/GFX0 (redact serials)
- [ ] X6000 IORegistry on **same OS major** as bring-up (Sequoia or Tahoe)
- [ ] Firmware redistrib license checked

## Traces

[`docs/traces/sequoia-7950x3d/`](traces/sequoia-7950x3d/) — `system_profiler.txt`, `kextstat-gpu.txt`, `ioreg-igpu-excerpt.txt`, `ioreg-gfx0-rtx5080-excerpt.txt`, `opencore-inject-2026-09-07.txt`, `gop-info.txt`, `pci-display.txt`, `vfct-atom-connectors.txt`
