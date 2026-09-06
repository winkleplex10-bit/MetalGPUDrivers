# boards.md — Nvidia lab / freeze notes

**Phase 1 product board remains Ampere** (see [CURSOR-START-NVIDIA.md](../../../docs/CURSOR-START-NVIDIA.md) §3).  
This file also records **lab-enumerated** GSP-era cards present on the Hackintosh, including Blackwell.

## Lab board — RTX 5080 (Blackwell, not Phase 1)

Measured 2026-09-06 from Sequoia dump. Traces: [`traces/sequoia-rtx5080/`](traces/sequoia-rtx5080/).

| Field | Value |
|---|---|
| Product | **GeForce RTX 5080** (consumer Blackwell / GB20x) |
| PCI VID:DID | **`10de:2c02`** |
| PCI revision | **`0xA1`** |
| Subsystem | **`1462:5315`** (MSI) |
| PCI BDF | **`1:0:0`** (`pcidebug`) |
| IOKit nub | **`GFX0@0`** (`IOPCIDevice`) |
| ACPI path | **`_SB.PCI0.GPP0.VGA`** (`IOACPIPlane:/_SB/PCI0@0/GPP0@10001/VGA@0`) |
| `compatible` | `pci1462,5315`, `pci10de,2c02`, `pciclass,030000`, `VGA`, `GFX0` |
| Slot | **Slot-1** (`AAPL,slot-name`) |
| Link | PCIe x16 (System Information) |
| FB today | **`IONDRVFramebuffer`** child present; **no** Nvidia Metal / Resman kext |
| System Information | Vendor NVIDIA `0x10de`, Device ID `0x2c02`, Rev `0xA1`, **No Kext Loaded** |
| Host OS | macOS Sequoia **15.7.8 (24G824)** |
| SMBIOS | MacPro7,1 |
| Co-GPU on same host | Raphael iGPU `1002:164E` @ `IGPU@0` (main 4K display) — see `ihv/amd-rdna2-igpu/` |
| Lilu / WEG | Lilu 1.7.2; WhateverGreen 1.7.1d7 (laobamac) |

### What this proves

- Blackwell consumer DID **`0x2c02`** enumerates cleanly on Sequoia Hackintosh PCIe.
- Card appears in System Information **without** a vendor Metal stack (expected).
- Dual-GPU with AMD Raphael iGPU: both nubs live; iGPU owns main display; 5080 is secondary/PCI-only.
- WhateverGreen loaded does not need to be removed for this card to remain visible.

### What this does **not** prove

- GSP boot, modeset, or Metal on Blackwell under macOS
- That RTX 5080 is the Phase 1 bring-up card (Phase 1 stays **Ampere**)
- Dual `MTLCopyAllDevices` with Nvidia (no Metal plugin yet)

### Match hygiene (when Blackwell is added to allow-list)

```
IOPCIPrimaryMatch = 0x2c0210de
```

Never use class-only `0x03000000` match — that would collide with the Raphael iGPU and any other VGA.

## Phase 1 freeze (Ampere) — still empty

| Field | Value |
|---|---|
| Frozen Ampere DID | UNKNOWN — pick one GA10x from open-rm table when board exists |
| Host for Ampere bring-up | UNKNOWN |

Until an Ampere card is frozen, treat the RTX 5080 row as **Blackwell lab inventory only**.
