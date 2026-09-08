# AMD RDNA2 iGPU — Raphael / Rembrandt (NootedRed gap)

UMA APU backend for AMD RDNA2 integrated GPUs that **NootedRed does not support**. First board: Ryzen 7000 desktop Raphael iGPU (e.g. 7950X3D), LLVM `gfx1036`, PCI DID `0x164E`.

**Binding specs:**
- [docs/CURSOR-START-AMD-RDNA2-IGPU.md](../../docs/CURSOR-START-AMD-RDNA2-IGPU.md) — living plan (updated 7 Sep 2026)
- [docs/CURSOR-START-AMD.md](../../docs/CURSOR-START-AMD.md) — shared AMD host contracts / do-nots

## Why this slot exists

| Stack | What it covers | Gap |
|---|---|---|
| **NootedRed** ([ChefKiss](https://chefkiss.dev/applehax/nootedred/)) | Vega / GCN 5 Raven family — Ryzen 1xxx–5xxx APUs and **7x30** (reused GCN 5) | Explicitly **not** RDNA2 ([discussion #345](https://github.com/ChefKissInc/NootedRed/discussions/345)) |
| **Apple `AMDRadeonX6000*`** | Discrete Navi / RDNA1–2 Metal on Intel macOS | Never an APU match path for Raphael |
| **This IHV** | Own FB + accelerator + Metal plugin for RDNA2 **iGPU** form factor | Fill the NootedRed RDNA2 hole without Lilu/Apple-blob enablement |

## Scope

- **First target:** Raphael (Ryzen 7000 AM5) — Radeon Graphics, 2 CU, `gfx1036`, DID `0x164E`
- **Named first SKU / lab board:** Ryzen 9 **7950X3D**, DID `1002:164E` rev `C9`, nub `IGPU@0`, ACPI `_SB.PCI0.GP17.VGA` (Sequoia dump in `docs/traces/sequoia-7950x3d/`)
- **Later sibling:** Rembrandt (Ryzen 6000 mobile RDNA2, `gfx1035`) — same backend shape, separate Phase 0 freeze
- **Coexistence (product):** **macOS 26**, **WhateverGreen absent**; **discrete GPUs** may stay enabled. Primary display is **connector-driven**. See [docs/coexistence.md](docs/coexistence.md), plan §2.1, and [docs/BUILD-RULES.md](../../docs/BUILD-RULES.md).
- **Lab evidence:** Monterey/Sequoia unaccelerated iGPU boot; System Information listed iGPU + **RTX 5080** (PCI coexistence; no Nvidia Metal). Details in [docs/boards.md](docs/boards.md). Sequoia dumps that show WEG are historical only.
- **Not:** Vega Raven / Cezanne / 7x30 (use NootedRed or leave alone)
- **Not:** Hawk Point 700M / Phoenix (`gfx1103` RDNA3) — closer to `ihv/amd-rdna3*` / future `amd-rdna3-igpu`
- **Not:** Strix Point 800M RDNA 3.5 — `ihv/amd-rdna35-igpu/`
- **Not:** installing WhateverGreen or requiring `-wegnoegpu` for this product
- Matching is platform/ACPI-shaped UMA APU, **Raphael DID only** — never claim discrete Navi/RX DIDs

## Layout

```
kext/         RaphaelIGPU Info.plist + kmod; RaphaelFB-Info.plist (GOP wrap, separate kext)
match/        RaphaelController — DID 0x164E only; category RaphaelHW (beside AMDSupport)
display/      ATOM/VFCT parser, GOP-wrap IOFramebuffer, extra connector nubs
submit/       RaphaelAccelerator stub (no IOAccel user clients; Metal plugin off by default)
metal/        RaphaelMTLDriver.bundle plist stub — do not enable raphael_metal=1
firmware/     PSP/GC/DCN names; blobs not in git
docs/         boards, coexistence, unknowns, BUILD.md, traces
```

Build / load notes (no OpenCore or SIP recipes): [docs/BUILD.md](docs/BUILD.md).

## Phase status

| Phase | Status |
|---|---|
| R0 board freeze | Mostly done (Sequoia dump + 7 Sep SysReport). Open: board SKU, which APU jack is 4K, X6000 oracle, firmware license |
| R1 enumerate | In tree — BAR map + ATOM/VFCT parser; Boot KC inject of v0.1.0 **failed** (`IOGraphicsFamily`). Controller-only kext should inject; attach unverified on box |
| R2 dumb FB | In tree as `RaphaelFB.kext` — GOP wrap vs IONDRV; extra HDMI/DP offline; cannot prelink without IOGraphicsFamily in the same KC |
| R3–R6 compute / Metal / present | Not started |

**Honest Sequoia outcome for this drop:** `RaphaelIGPU.kext` is meant to attach `RaphaelController` on `IGPU@0` without taking the screen. GOP wrap is `RaphaelFB.kext` and still needs `IOGraphicsFamily` in the same kernel collection. Neither is video acceleration. Metal/QE needs IOGPU user clients + DCN 3.1.5 modeset + AIR→gfx1036.

## Relation to NootedRed (style vs method)

**Style (keep):** single-purpose AMD iGPU enablement kext stack; Hackintosh-on-AMD-APU platform; FB then acceleration; honest device identity.

**Method (do not copy):** Lilu-style patching of Apple AMD kexts, DID spoof onto `AMDRadeonX6000*`, WhateverGreen-class blob enablement, or NootedRed’s “remove WEG / disable dGPU” install rules. This repo’s product path is a **new IHV backend** behind `host/` that runs on **macOS 26 without WEG** and leaves discrete cards alone.

**Oracle advantage:** Apple *did* ship RDNA2 discrete Metal. Trace `AMDRadeonX6000` on Tahoe for IOGPU / `MetalPluginName` ABI. Still implement **our** match, UMA, DCN 3.1.5, and 2 CU limits — do not attach Apple’s Navi personality to Raphael, and do not hijack the discrete card’s stack.
