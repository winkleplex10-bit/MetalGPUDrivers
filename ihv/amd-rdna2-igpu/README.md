# AMD RDNA2 iGPU — Raphael / Rembrandt (NootedRed gap)

UMA APU backend for AMD RDNA2 integrated GPUs that **NootedRed does not support**. First board: Ryzen 7000 desktop Raphael iGPU (e.g. 7950X3D), LLVM `gfx1036`, PCI DID `0x164E`.

**Binding specs:**
- [docs/CURSOR-START-AMD-RDNA2-IGPU.md](../../docs/CURSOR-START-AMD-RDNA2-IGPU.md) — this slot’s plan
- [docs/CURSOR-START-AMD.md](../../docs/CURSOR-START-AMD.md) — shared AMD host contracts / do-nots

## Why this slot exists

| Stack | What it covers | Gap |
|---|---|---|
| **NootedRed** ([ChefKiss](https://chefkiss.dev/applehax/nootedred/)) | Vega / GCN 5 Raven family — Ryzen 1xxx–5xxx APUs and **7x30** (reused GCN 5) | Explicitly **not** RDNA2 ([discussion #345](https://github.com/ChefKissInc/NootedRed/discussions/345)) |
| **Apple `AMDRadeonX6000*`** | Discrete Navi / RDNA1–2 Metal on Intel macOS | Never an APU match path for Raphael |
| **This IHV** | Own FB + accelerator + Metal plugin for RDNA2 **iGPU** form factor | Fill the NootedRed RDNA2 hole without Lilu/Apple-blob enablement |

## Scope

- **First target:** Raphael (Ryzen 7000 AM5) — Radeon Graphics, 2 CU, `gfx1036`, DID `0x164E`
- **Named first SKU:** Ryzen 9 7950X3D (same Raphael iGPU as 7950X / 7900X / …)
- **Later sibling:** Rembrandt (Ryzen 6000 mobile RDNA2, `gfx1035`) — same backend shape, separate Phase 0 freeze
- **Coexistence (required):** stock **WhateverGreen** may stay loaded; **discrete GPUs** may stay enabled. Primary display is **connector-driven** (cable on APU ports → our FB; cable on dGPU → dGPU FB). See [docs/coexistence.md](docs/coexistence.md) and plan §2.1.
- **Lab evidence:** Monterey unaccelerated iGPU boot; System Information listed iGPU + **RTX 5080** (PCI coexistence; no Nvidia Metal). Details in [docs/boards.md](docs/boards.md).
- **Not:** Vega Raven / Cezanne / 7x30 (use NootedRed or leave alone)
- **Not:** Hawk Point 700M / Phoenix (`gfx1103` RDNA3) — closer to `ihv/amd-rdna3*` / future `amd-rdna3-igpu`
- **Not:** Strix Point 800M RDNA 3.5 — `ihv/amd-rdna35-igpu/`
- **Not:** requiring `-wegnoegpu` or removal of WhateverGreen (NootedRed’s install model)
- Matching is platform/ACPI-shaped UMA APU, **Raphael DID only** — never claim discrete Navi/RX DIDs

## Layout

```
firmware/     PSP/MP0 13.0.5, GC 10.3.6, DCN 3.1.5 (blobs not in git unless licensed)
match/        APU/platform match — Raphael DID 0x164E only; not a Navi 21 / dGPU table
display/      APU DCN 3.1.5; connector-driven primary vs dGPU
submit/       PM4 / GFX10.3 compute classes (trace X6000 for IOGPU shape)
isa/          AIR → gfx1036 (and later gfx1035); closest Oracle = live X6000 RDNA2
uma/          Shared-memory / IOSurface rules for 2 CU UMA
docs/         boards.md, unknowns.md, coexistence.md, traces
```

## Relation to NootedRed (style vs method)

**Style (keep):** single-purpose AMD iGPU enablement kext stack; Hackintosh-on-AMD-APU platform; FB then acceleration; honest device identity.

**Method (do not copy):** Lilu-style patching of Apple AMD kexts, DID spoof onto `AMDRadeonX6000*`, WhateverGreen-class blob enablement, or NootedRed’s “remove WEG / disable dGPU” install rules. This repo’s product path is a **new IHV backend** behind `host/` that **coexists** with WEG and discrete cards.

**Oracle advantage:** Apple *did* ship RDNA2 discrete Metal. Trace `AMDRadeonX6000` on Tahoe for IOGPU / `MetalPluginName` ABI. Still implement **our** match, UMA, DCN 3.1.5, and 2 CU limits — do not attach Apple’s Navi personality to Raphael, and do not hijack the discrete card’s stack.

## Phase status

Not started. See [CURSOR-START-AMD-RDNA2-IGPU.md](../../docs/CURSOR-START-AMD-RDNA2-IGPU.md) §7 for R0–R6 acceptance criteria. Preferred after RDNA3 dGPU P0–P2 prove `host/` FB/accel shells, but may start Phase 0 in parallel (ABI oracle + board freeze only).
