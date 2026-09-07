# CURSOR-IHV-DRIVER-SPEC — host contract (stub)

**Status:** Stub. Binding vendor starters: `CURSOR-START-*.md`.  
**Date:** 2026-09-07.

This file is the vendor-agnostic host contract referenced by IHV slots. Full research corpus (`01-metal-userspace.md`, `02-kernel-boot-display.md`, `03-nvidia-prior-art.md`, `FINDINGS.md`) is still pending; do not invent IOGPU selectors here.

---

## 1. Goal (host)

Apps keep linking Apple `Metal.framework`. WindowServer keeps using `IOFramebuffer`. We supply:

- `host/iofb/` — linear `IOFramebuffer` shell
- `host/ioaccel/` — accelerator shell speaking IOGPU / IOAccel user clients (traced, not guessed)
- `host/mtl-plugin/` — `*MTLDriver.bundle` skeleton (`MetalPluginName` / `MetalPluginClassName`)
- `host/air/` — `.metallib` / AIR ingest helpers
- `host/present/` — IOSurface present glue
- `host/power/` — surprise-remove / MSI helpers

IHV directories under `ihv/*` implement firmware, display engine, submit, and AIR→ISA.

## 2. Platform pin

| Item | Value |
|---|---|
| Preferred host | Intel x86 Mac or Hackintosh; 2019 Mac Pro PCIe preferred for dGPU |
| Long-term OS pin | macOS 26 Tahoe |
| Current Nvidia lab OS | Sequoia 15.7.9 (24G830) — document deviations in IHV `boards.md` |
| Loading | Assumed solved (OpenCore / AuxKC). No SIP recipes in this tree |

## 3. Host phases (summary)

| Phase | Name | Host deliverable |
|---|---|---|
| P0 | Freeze | Bundle IDs, board file, UNKNOWN list |
| P1 | Enumerate + firmware | PCI attach hooks; IHV firmware alive |
| P2 | Dumb FB | `IOFBLinearShell` + IHV modeset |
| P3 | Non-Metal compute | BO + submit path without Metal.framework |
| P4 | AIR→ISA offline | Compiler before `MTLDevice` |
| P5 | MTLDevice | Plugin registry + compute PSO |
| P6 | Present | `CAMetalLayer` / IOSurface scanout |
| P7 | Generation expand | New DIDs inside existing IHV slot |

Vendor acceptance IDs (N1.x, R1.x, …) live in each `CURSOR-START-*.md`.

## 4. Bundle ID policy

- Project-owned prefixes only (e.g. `dev.metalgpudrivers.*`)
- Never `com.apple.*`, never historical `com.nvidia.web.*` / Kepler IDs

Frozen examples:

| Bundle | Role |
|---|---|
| `dev.metalgpudrivers.RaphaelIGPU` | AMD Raphael enumerate |
| `dev.metalgpudrivers.NvidiaGSP` | Nvidia GB203 N1 enumerate |

## 5. Hard rules

1. Do not fork `Metal.framework`, `IOGPU.framework`, or WindowServer
2. Do not load Linux KMDs (`nvidia.ko`, `amdgpu.ko`, …) on XNU
3. Do not spoof Apple personalities across generations
4. If a selector / RPC / opcode is not traced or in an opened public header: write **UNKNOWN** and stop

## 6. Open host questions

Carry forward from vendor starters until measured on the pinned OS:

1. IOGPU vs `IOAccelContext2` on Sequoia/Tahoe Intel discrete
2. Whether `CompilerPluginInterface` accepts non-Apple AIR backends
3. Third-party `IOGraphicsFamily` KPI status on this SDK
4. Cross-device IOSurface without CPU
