# boards.md — Nvidia Phase 0 freeze (lab)

**Frozen board (this lab):** GeForce RTX 5080 — consumer Blackwell **GB203**, DID **`0x2c02`**.

CURSOR-START-NVIDIA §3 prefers Ampere for Phase 1. This lab has no Ampere card. We freeze the **RTX 5080** as the Phase 0/1 board and use **GB20x** generation tables from the start. Same GSP + open-rm IHV slot; open-rm is **mandatory** on Blackwell.

## Frozen board — RTX 5080 (Blackwell GB20x)

Measured 2026-09-07 on this host. Traces: [`traces/sequoia-rtx5080/`](traces/sequoia-rtx5080/).

| Field | Value |
|---|---|
| Product | **GeForce RTX 5080** (GB203 / GB20x) |
| PCI VID:DID | **`10de:2c02`** (open-rm Compatible GPUs) |
| PCI revision | **`0xA1`** |
| Subsystem | **`1462:5315`** (MSI) |
| HDAU function | **`10de:22e9`** — do not claim |
| PCI BDF | **`1:0:0`** (`pcidebug`) |
| IOKit nub | **`GFX0@0`** |
| ACPI path | **`_SB.PCI0.GPP0.VGA`** |
| `compatible` | `pci1462,5315`, `pci10de,2c02`, `pciclass,030000`, `VGA`, `GFX0` |
| Slot | **Slot-1** |
| Link | PCIe **x16 @ 32 GT/s** (`IOPCIExpressLinkStatus=0x1105`) |
| BARs (IODeviceMemory) | `0xd8000000` 64 MiB; `0xf2000000` 16 B; `0xf0000000` 32 MiB; `0xdc000000` 512 KiB |
| FB today | **`IONDRVFramebuffer`** (probe 20000); System Information: **No Kext Loaded** |
| Host OS | macOS Sequoia **15.7.9 (24G830)** — not Tahoe yet |
| SMBIOS | MacPro7,1 |
| Co-GPU | Raphael iGPU `1002:164E` — [`ihv/amd-rdna2-igpu/`](../../amd-rdna2-igpu/), see [coexistence.md](coexistence.md) |
| Our kext (N1) | `dev.metalgpudrivers.NvidiaGSP` / `NvidiaController` |

### Match string (frozen)

```
IOPCIPrimaryMatch = 0x2c0210de
```

### Deviation log

| Spec default | Lab decision |
|---|---|
| Ampere first | Blackwell 5080 — only discrete Nvidia present |
| macOS 26 Tahoe pin | Sequoia 15.7.9 lab OS; Tahoe remains the long-term pin |
| GSP blobs in-tree | Never — build-time only; macOS redistrib **UNKNOWN** |

## Ampère alternate (not available here)

Leave empty until a GA10x board exists. Do not invent DIDs.
