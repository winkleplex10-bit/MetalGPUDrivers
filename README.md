# macOS GPU IHV Drivers

Professional-grade unofficial GPU drivers for macOS on hardware Apple never shipped as Metal devices — including an **Apple Silicon display-acceleration** track.

This repository implements **IHV (Independent Hardware Vendor) backends** behind a shared, vendor-agnostic **host** layer. Applications keep using Apple's `Metal.framework`, WindowServer, and `IOGraphicsFamily`; we fill the vendor slot with new kexts, firmware bring-up, display programming, command submission, and AIR→ISA compilers.

## Status

**Raphael iGPU kext started** on x86. `RaphaelIGPU.kext` enumerates `1002:164E` (ATOM/VFCT HDMI+DP). GOP wrap is a second kext (`RaphaelFB`) that still needs `IOGraphicsFamily` in the same kernel collection. **Not** Sequoia QE/Metal (no GFX ring, no AIR→gfx1036). Other IHV slots remain plan-only. **Apple Silicon track:** scaffolded for display acceleration — challenges documented, **not** blocked.

## Build rules (binding)

See **[docs/BUILD-RULES.md](docs/BUILD-RULES.md)**. Summary:

| Rule | Value |
|---|---|
| **Target OS** | **macOS 26** (Tahoe) |
| **WhateverGreen** | **Absent** — product stack does not load WEG |
| **x86 track** | Intel Mac / Hackintosh IHVs (`ihv/amd-*`, `nvidia`, `arc`, …) |
| **ARM track** | Apple Silicon display acceleration (`ihv/apple-silicon/`) — hard problems listed, track stays open |

## Target platform

- **OS pin:** macOS 26
- **x86 hosts:** Intel Mac or Hackintosh (2019 Mac Pro preferred for discrete GPUs; Raphael lab is AM5)
- **ARM hosts:** Apple Silicon Macs — external display acceleration first; DCP/lid issues are tracked challenges
- **Not in product docs:** SIP/OpenCore/unsigned-kext recipes

## Repository layout

```
host/                 Vendor-agnostic macOS integration (IOFramebuffer shell, IOGPU glue, MTL plugin skeleton)
ihv/nvidia/           Ampere+ GSP-era Nvidia
ihv/amd-rdna3/        RDNA3 / RX 7000 — plan-only
ihv/amd-rdna4/        RDNA4 / RX 9000
ihv/amd-rdna2-igpu/   RDNA2 APU (Raphael) — controller kext started; GOP wrap split
ihv/amd-rdna35-igpu/  RDNA 3.5 APU (Strix Point 800M)
ihv/arc/              Intel Arc discrete (Xe-HPG / Xe2)
ihv/intel-igpu/       Modern Intel iGPU (Xe-LPG / Xe2-LPG)
ihv/apple-silicon/    ARM / Apple Silicon — display acceleration track
docs/                 Architecture specs, BUILD-RULES, phased plans
```

## Documentation

| Document | Purpose |
|---|---|
| [docs/BUILD-RULES.md](docs/BUILD-RULES.md) | **Binding** OS pin, WEG policy, x86 vs ARM tracks |
| [docs/CURSOR-START-APPLE-SILICON.md](docs/CURSOR-START-APPLE-SILICON.md) | Apple Silicon display-acceleration plan |
| [docs/CURSOR-START-AMD.md](docs/CURSOR-START-AMD.md) | AMD RDNA3/RDNA4/RDNA 3.5 starter (plan-only in-tree) |
| [docs/CURSOR-START-AMD-RDNA2-IGPU.md](docs/CURSOR-START-AMD-RDNA2-IGPU.md) | Raphael RDNA2 iGPU — living plan; GOP-wrap kext started |
| [docs/CURSOR-START-NVIDIA.md](docs/CURSOR-START-NVIDIA.md) | Nvidia Ampere+ GSP-era starter |
| [docs/CURSOR-START-INTEL.md](docs/CURSOR-START-INTEL.md) | Intel Arc + modern iGPU starter |

**Pending:** `CURSOR-IHV-DRIVER-SPEC.md` (host contract), research corpus (`01-metal-userspace.md`, `02-kernel-boot-display.md`, `03-nvidia-prior-art.md`, `FINDINGS.md`).

## Phased approach

All vendors follow host phases (0–6): freeze board → enumerate + firmware alive → dumb framebuffer → non-Metal compute → AIR→ISA compile → MTLDevice → present. Apple Silicon uses AS0–AS7 (external heads before lid/DCP). Vendor-specific acceptance criteria are in each CURSOR-START doc.

**What actually started:** Raphael iGPU (`ihv/amd-rdna2-igpu/`) because the lab board is a 7950X3D. Product re-validation target is **macOS 26 without WhateverGreen**.

## Hard rules

- Follow [docs/BUILD-RULES.md](docs/BUILD-RULES.md)
- Do **not** fork `Metal.framework`, `IOGPU.framework`, or WindowServer
- Do **not** spoof Apple kext personalities
- Do **not** port Linux KMDs onto XNU
- Do **not** treat Apple Silicon display work as impossible or out of scope
- If a selector, entitlement, or opcode is not in public docs or traced headers: write **UNKNOWN** and stop

## License

TBD. Firmware blobs are **not** committed unless a macOS redistribution grant is documented.
