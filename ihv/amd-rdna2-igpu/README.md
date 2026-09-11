# AMD RDNA2 iGPU — Raphael / Rembrandt (NootedRed gap)

UMA APU backend for AMD RDNA2 integrated GPUs that **NootedRed does not support**. First board: Ryzen 7000 desktop Raphael iGPU (e.g. 7950X3D), LLVM `gfx1036`, PCI DID `0x164E`.

**Binding specs:**
- [docs/CURSOR-START-AMD-RDNA2-IGPU.md](../../docs/CURSOR-START-AMD-RDNA2-IGPU.md) — living plan (updated 10 Sep 2026, **0.2.7** on-box)
- [docs/CURSOR-START-AMD.md](../../docs/CURSOR-START-AMD.md) — shared AMD host contracts / do-nots

## Why this slot exists

| Stack | What it covers | Gap |
|---|---|---|
| **NootedRed** ([ChefKiss](https://chefkiss.dev/applehax/nootedred/)) | Vega / GCN 5 Raven family — Ryzen 1xxx–5xxx APUs and **7x30** (reused GCN 5) | Explicitly **not** RDNA2 ([discussion #345](https://github.com/ChefKissInc/NootedRed/discussions/345)) |
| **Apple `AMDRadeonX6000*`** | Discrete Navi / RDNA1–2 Metal on Intel macOS | Never an APU match path for Raphael |
| **This IHV** | Own FB + accelerator + Metal plugin for RDNA2 **iGPU** form factor | Fill the NootedRed RDNA2 hole without Lilu/Apple-blob enablement |

## Scope

- **First target:** Raphael (Ryzen 7000 AM5) — Radeon Graphics, 2 CU, `gfx1036`, DID `0x164E`
- **Named first SKU / lab board:** Ryzen 9 **7950X3D**, DID `1002:164E` rev `C9`. Tahoe nub **`VGA@0`** under `GP17@8,1` (`pci1002,164e`). Sequoia dump used `IGPU@0` (historical, `docs/traces/sequoia-7950x3d/`). **Current OS is Tahoe, no WEG.**
- **Later sibling:** Rembrandt (Ryzen 6000 mobile RDNA2, `gfx1035`) — same backend shape, separate Phase 0 freeze
- **Coexistence:** **discrete GPUs** stay enabled (DID-only match). **WhateverGreen is not required** and is **not loaded** on the Tahoe lab. Primary display is **connector-driven**. See [docs/coexistence.md](docs/coexistence.md) and plan §2.1.
- **Lab evidence:** Tahoe 26 (build **25G83**), no WEG. AuxKC from `/Library/Extensions` (runtime `kmutil load` is too late vs IONDRV). R1 attach + R2 GOP wrap on `VGA@0` (0.2.3 colors honest). BAR5 `phys=0xdd600000 size=524288 map=1`. Live jack **DP** (user; VFCT HDMI-A `0x320C` path 0 vs DP `0x3113` path 2 — names disagree with silkscreen). **0.2.7:** `raphael_dcn_modeset=1` GPINT (`GET_FW_VERSION=0x05000649`) + OTG0 4K reaffirm on live HPD2/OTG0; `RaphaelDmubOk=Yes`. 5080 IONDRV untouched. Sequoia traces remain historical. Details in [docs/boards.md](docs/boards.md).
- **Not:** Vega Raven / Cezanne / 7x30 (use NootedRed or leave alone)
- **Not:** Hawk Point 700M / Phoenix (`gfx1103` RDNA3) — closer to `ihv/amd-rdna3*` / future `amd-rdna3-igpu`
- **Not:** Strix Point 800M RDNA 3.5 — `ihv/amd-rdna35-igpu/`
- **Not:** requiring `-wegnoegpu`, requiring WEG to be present, or requiring WEG removal (NootedRed’s install model)
- Matching is platform/ACPI-shaped UMA APU, **Raphael DID only** — never claim discrete Navi/RX DIDs

## Layout

```
kext/         Info.plist + kmod start/stop — bundle ID dev.metalgpudrivers.RaphaelIGPU
match/        RaphaelController — DID 0x164E only; category RaphaelHW (beside AMDSupport)
display/      ATOM parser, GOP-wrap IOFramebuffer, extra connector nubs, DCN regs / DMUB / OTG reaffirm
submit/       RaphaelAccelerator stub (no IOAccel user clients; Metal plugin off by default)
metal/        RaphaelMTLDriver.bundle plist stub — do not enable raphael_metal=1
firmware/     PSP/GC/DCN names + LICENSE.amdgpu; blobs not in git
docs/         boards, coexistence, unknowns, BUILD.md, traces
```

Build / load notes (no OpenCore or SIP recipes): [docs/BUILD.md](docs/BUILD.md).

## Phase status

| Phase | Status |
|---|---|
| R0 board freeze | PCI identity from Sequoia dump; **bring-up Tahoe, no WEG**. Open: board SKU, X6000 oracle. Physical jack **DP** (user). Tahoe `ioreg`/`kextstat` captured for R1/R2. |
| R1 enumerate | **On-box.** `RaphaelController` on `VGA@0` `1002:164e`, `RaphaelMap=No`. **0.2.4:** connectors=2 (HDMI, DP). AMDSupport sibling. 0.2.6+ bundles DMCUB/PSP at build time. |
| R2 dumb FB | **On-box wrap (0.2.3 colors honest).** WindowServer `fb0=/RaphaelFramebuffer`. **0.2.7:** boot head **DP** index 1; controller `RaphaelPhase=R2-dcn-modeset`; FB still `R2-gop-wrap` + `connector-kind=DP`. Handshake + 4K reaffirm, not a new timing. **Next:** first changing modeset ([plan §11](../../docs/CURSOR-START-AMD-RDNA2-IGPU.md)). |
| R3–R6 compute / Metal / present | Not started |

**Honest Tahoe outcome (0.2.7):** WindowServer owns our `IOFramebuffer` on the GOP head when the kexts are in AuxKC **before** IONDRV claims `IOFramebuffer`. That is **not** video acceleration. GOP phys is a PCI BAR (`0x10000000000`). 0.2.7 GPINTs GOP DMCUB (`dal_fw=0` is not abort) and reaffirms live OTG0 4K (`H_TOTAL=0xf9f V_TOTAL=0x8ad`, CTL unchanged). Metal/QE still needs IOGPU user clients + AIR→gfx1036. Do not re-gate GPINT on `dal_fw`.

## Relation to NootedRed (style vs method)

**Style (keep):** single-purpose AMD iGPU enablement kext stack; Hackintosh-on-AMD-APU platform; FB then acceleration; honest device identity.

**Method (do not copy):** Lilu-style patching of Apple AMD kexts, DID spoof onto `AMDRadeonX6000*`, WhateverGreen-class blob enablement, or NootedRed’s “remove WEG / disable dGPU” install rules. This repo’s product path is a **new IHV backend** behind `host/` that owns Raphael by DID and leaves other GPUs alone, with or without WEG.

**Oracle advantage:** Apple *did* ship RDNA2 discrete Metal. Trace `AMDRadeonX6000` on Tahoe for IOGPU / `MetalPluginName` ABI. Still implement **our** match, UMA, DCN 3.1.5, and 2 CU limits — do not attach Apple’s Navi personality to Raphael, and do not hijack the discrete card’s stack.
