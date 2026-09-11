# boards.md — Phase R0 board freeze

**Status:** PCI/ACPI identity from Sequoia dump `docs/traces/sequoia-7950x3d/` (2026-09-06). **Bring-up OS: macOS 26 Tahoe, WhateverGreen absent, build 25G83** ([BUILD-RULES.md](../../../docs/BUILD-RULES.md)). Sequoia traces are historical. Living plan: [`docs/CURSOR-START-AMD-RDNA2-IGPU.md`](../../../docs/CURSOR-START-AMD-RDNA2-IGPU.md) §0.

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
| IOKit nub name | **Tahoe: `VGA@0`**. Sequoia dump: `IGPU@0`. Same DID; match is DID-only. |
| ACPI path | **`_SB.PCI0.GP17.VGA`** (`IOACPIPlane:/_SB/PCI0@0/GP17@80001/VGA@0`) |
| `compatible` | `pci1043,8877`, `pci1002,164e`, `pciclass,030000`, `VGA`, `IGPU` |
| Parent bridge | `GP17@8,1` under `PCI0` |
| Current FB | **Tahoe 0.2.7:** `RaphaelFramebuffer` (`R2-gop-wrap`, `connector-kind=DP`) on `VGA@0`; controller `RaphaelPhase=R2-dcn-modeset`, `RaphaelDmubOk=Yes`. Pre-wrap / late load: `IONDRVFramebuffer` (`.Display_boot`). |
| Also attached | **`AMDSupport`** (Apple, probe 65050, vendor-wide AMD VGA match) — not a Metal stack |
| APU display | **Main display** 3840×2160@HiDPI; System Information: VRAM 31 MB, **No Kext Loaded** (acceleration) |
| Discrete GPU | **Nvidia RTX 5080** `10de:2c02` rev `0xA1`, subsystem `1462:5315` (MSI), nub **`GFX0@0`**, BDF `1:0:0`, Slot-1 — also filed under [`ihv/nvidia/docs/boards.md`](../../nvidia/docs/boards.md) |
| dGPU ACPI | `_SB.PCI0.GPP0.VGA` |
| Lilu (Sequoia dump only) | **1.7.2** — historical; **not** assumed on Tahoe |
| WhateverGreen | **Not loaded** on current Tahoe lab. Sequoia dump had 1.7.1d7 laobamac — historical |
| `-wegnoegpu` / iGPU disable | Not used; iGPU is main display |
| Primary display policy | **Connector-driven** (product); currently APU owns main display |
| Lab OS (current) | **macOS 26 Tahoe**, Darwin 25.6.0, `OS Build Version` **25G83** |
| Lab OS (historical dump) | macOS Sequoia 15.7.8 (24G824) |
| Product OS pin | **macOS 26 Tahoe** |
| X6000 oracle | **Still needed** — 5080 cannot provide AMD Metal ABI |
| Firmware license | **LICENSE.amdgpu** — binary redistrib, no RE; `dcn_3_1_5_dmcub.bin` + `psp_13_0_5_{toc,ta}` fetched at build (see [firmware/README.md](../firmware/README.md)) |
| First kexts | `dev.metalgpudrivers.RaphaelIGPU` + `dev.metalgpudrivers.RaphaelFB` **0.2.7** — see [BUILD.md](BUILD.md). AuxKC from `/Library/Extensions` (lab fact). |

## Match personality (from dump)

```
IOPCIPrimaryMatch  = 0x164E1002
IOPCIRevisionMatch = 0xC9??????   (optional; rev C9 observed)
IONameMatch        = display      (optional; both GPUs use IOName display — prefer PCI match only)
```

**Do not** match `IOPCIClassMatch` `0x03000000` alone — that would also catch the RTX 5080’s VGA class and other AMD cards. **DID-only** match on `0x164E`.

## Lab evidence

- Sequoia dump proved dual-GPU enumeration: Raphael iGPU + RTX 5080.
- Tahoe **0.2.2** on-box: R1 `RaphaelController` (`map=0`, `R1-enumerate`) and R2 `RaphaelFramebuffer` (`R2-gop-wrap`, 4K, WindowServer `fb0`). iGPU NDRV gone; 5080 NDRV remains. AMDSupport sibling.
- Console GOP geometry matches NDRV (`33177600` bytes). `getConsoleInfo` `v_baseAddr=0x10000000001` is not a physical — blue tint on 0.2.2. v0.2.3 uses PCI BAR (`phys=0x10000000000`). v0.2.4: VFCT HDMI+DP, not BuiltIn; **BAR5 probe on-box** (`phys=0xdd600000 size=524288 map=1`, HPD2 `raw=0x00000012`). v0.2.5: named HPD decode + OTG dump. **Physical 4K jack is DP** (user); VFCT path 0 HDMI-A disagrees. **0.2.7:** `dcn_modeset=1 boot_jack=DP`; `DP index 1 boot=1`; live OTG0 HPD2 SENSE; GPINT `0x05000649`; OTG reaffirm same GOP totals.
- Apple **AMDSupport** still on the iGPU. Do not fight it globally.

## Acceptance (R0)

- [x] Exact DID/rev + IORegistry path captured
- [x] Discrete companion identified (RTX 5080)
- [x] Unaccelerated iGPU desktop observed (Sequoia + Tahoe GOP wrap)
- [x] Historical WEG/Lilu versions recorded (Sequoia). Tahoe lab: **no WEG**
- [x] Tahoe `kextstat` + `ioreg` (R1/R2 0.2.2). Build **25G83**.
- [x] Physical APU port: **DisplayPort** (user). VFCT still lists HDMI-A then DP.
- [ ] X6000 IORegistry on **Tahoe** (same OS major as bring-up)
- [x] Firmware redistrib license checked (`LICENSE.amdgpu` for DMCUB/PSP)

## Traces

Historical: [`docs/traces/sequoia-7950x3d/`](traces/sequoia-7950x3d/). Tahoe wrap was captured live (kextstat/ioreg/log); not yet filed as a trace dump in this folder.
