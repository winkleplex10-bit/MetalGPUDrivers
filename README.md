# macOS GPU IHV Drivers

Professional-grade unofficial GPU drivers for macOS on hardware Apple never shipped as Metal devices.

This repository implements **IHV (Independent Hardware Vendor) backends** behind a shared, vendor-agnostic **host** layer. Applications keep using Apple's `Metal.framework`, WindowServer, and `IOGraphicsFamily`; we fill the vendor slot with new kexts, firmware bring-up, display programming, command submission, and AIR→ISA compilers.

## Status

**Setup phase.** No driver code is implemented yet. Architecture, phased plans, and directory layout are defined in the docs below.

## Target platform

- **Host:** Intel x86 Mac or Hackintosh (2019 Mac Pro preferred for discrete GPUs)
- **OS pin:** macOS 26 Tahoe (last major Intel macOS)
- **Not in scope:** Apple Silicon display replacement; SIP/OpenCore/kext-loading recipes

## Repository layout

```
host/                 Vendor-agnostic macOS integration (IOFramebuffer shell, IOGPU glue, MTL plugin skeleton)
ihv/nvidia/           Ampere+ GSP-era Nvidia (Phase 1 IHV after AMD proof, or parallel track)
ihv/amd-rdna3/        RDNA3 / RX 7000 — first AMD target
ihv/amd-rdna4/        RDNA4 / RX 9000
ihv/amd-rdna2-igpu/   RDNA2 APU (Raphael / Rembrandt) — NootedRed gap
ihv/amd-rdna35-igpu/  RDNA 3.5 APU (Strix Point 800M)
ihv/arc/              Intel Arc discrete (Xe-HPG / Xe2)
ihv/intel-igpu/       Modern Intel iGPU (Xe-LPG / Xe2-LPG)
docs/                 Architecture specs and phased plans
```

## Documentation

| Document | Purpose |
|---|---|
| [docs/CURSOR-START-AMD.md](docs/CURSOR-START-AMD.md) | AMD RDNA3/RDNA4/RDNA 3.5 starter — **first implementation target** |
| [docs/CURSOR-START-AMD-RDNA2-IGPU.md](docs/CURSOR-START-AMD-RDNA2-IGPU.md) | Raphael RDNA2 iGPU (7950X3D) — NootedRed-unsupported APU plan |
| [docs/CURSOR-START-NVIDIA.md](docs/CURSOR-START-NVIDIA.md) | Nvidia Ampere+ GSP-era starter |
| [docs/CURSOR-START-INTEL.md](docs/CURSOR-START-INTEL.md) | Intel Arc + modern iGPU starter (Phase 7 slot) |

**Pending:** `CURSOR-IHV-DRIVER-SPEC.md` (host contract), research corpus (`01-metal-userspace.md`, `02-kernel-boot-display.md`, `03-nvidia-prior-art.md`, `FINDINGS.md`).

## Phased approach

All vendors follow the same host phases (0–6): freeze board → enumerate + firmware alive → dumb framebuffer → non-Metal compute → AIR→ISA compile → MTLDevice → present. Vendor-specific acceptance criteria are in each CURSOR-START doc.

**Recommended fill order:** AMD RDNA3 (`ihv/amd-rdna3/`) first — a live `AMDRadeonX6000` Metal stack exists on Tahoe Intel macOS to trace. Nvidia and Intel slots plug in without rewriting `host/`.

## Hard rules

- Do **not** fork `Metal.framework`, `IOGPU.framework`, or WindowServer
- Do **not** spoof Apple kext personalities (X6000 onto RDNA3, Kepler onto Ampere, etc.)
- Do **not** port Linux KMDs (`amdgpu.ko`, `nvidia.ko`, `i915.ko`) onto XNU
- If a selector, entitlement, or opcode is not in public docs or traced headers: write **UNKNOWN** and stop

## License

TBD. Firmware blobs are **not** committed unless a macOS redistribution grant is documented.
