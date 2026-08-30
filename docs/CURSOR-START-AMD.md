# CURSOR-START-AMD — unofficial IHV backends for RDNA3 / RDNA4 / RDNA 3.5

**Audience:** Cursor coding agents filling the AMD IHV slots defined by `CURSOR-IHV-DRIVER-SPEC.md`.  
**Product:** display scanout + Metal on GPUs Apple never shipped as Metal devices.  
**This file:** AMD-only starter. Three backends, **one host slot**. Do not rewrite `host/`.  
**Binding research (cite; do not invent APIs):** `CURSOR-IHV-DRIVER-SPEC.md`, `01-metal-userspace.md`, `02-kernel-boot-display.md`, `03-nvidia-prior-art.md`, `FINDINGS.md`.  
**Date:** 29 Aug 2026. Research corpus: 27 Aug 2026.

**Hard rules.** Architecture and implementation plan only. No SIP / OpenCore / AuxKC / unsigned-kext recipes. No kext-patch recipes. No RX 6000 personality spoof onto 7000 / 9000 / 800M. If a selector, entitlement, bundle ID, IOGPU method, PM4 opcode, or firmware RPC is not in the research files or a public header you have actually opened, write **UNKNOWN** and stop that branch.

---

## 1. Goal

Build an **unofficial IHV-quality GPU stack** so an AMD GPU Apple never shipped as a Metal device can do **display + Metal** on a machine where kexts can be loaded for development.

Three backends share the same host contracts (`CURSOR-IHV-DRIVER-SPEC` §5):

| Backend | Marketing | Arch | First-fill order | Repo |
|---|---|---|---|---|
| **RDNA3 dGPU** | RX 7000 / Navi 3x | GFX11 (`gfx1100`–`gfx1102`) | **First AMD target** | `ihv/amd-rdna3/` |
| **RDNA4 dGPU** | RX 9000 / Navi 4x | GFX12 (`gfx1200`/`gfx1201`) | Second — colder ISA | `ihv/amd-rdna4/` |
| **RDNA 3.5 iGPU** | Radeon 800M / 8000S on APUs | GFX11.5 (`gfx1150`+) | Third — UMA, display *is* the APU | `ihv/amd-rdna35-igpu/` |

**Done** is the same as the host spec (`CURSOR-IHV-DRIVER-SPEC` §3): WindowServer desktop on our `IOFramebuffer`, and `MTLCopyAllDevices()` returns our GPU running a stock `.metallib`. TinyGPU HIP compute, a triangle in a private harness, and spoofing an RX 6800 ID so `AMDRadeonX6000` attaches are **not-done**.

**Stance:** ridiculously difficult, **not** impossible. Easier than Nvidia because a **live** `AMDRadeonX6000` Metal stack exists to **TRACE** on the same OS. Still a **new IHV backend**: Apple’s discrete Metal kexts stop at RDNA2 (`01` §4.3; `03` §5.1). RDNA3 command set ≠ X6000. WhateverRed’s lesson: “adding support for non-existent logic is basically impossible” once the generation is purged ([WhateverRed](https://github.com/ainexur/WhateverRed)).

**Apple Silicon:** AGX stays the system GPU. DCP owns scanout (`02` §1.2). These AMD parts are **Intel-Mac dGPU** or **AMD-Hackintosh APU** problems, not an Apple Silicon story. A Thunderbolt RX 7000/9000 on an M-series Mac is a compute sidecar (TinyGPU already occupies that niche) and cannot replace DCP for the built-in panel ([102363](https://support.apple.com/en-us/102363)).

---

## 2. Host vs create

We do **not** replace Apple’s host. We **implement the IHV slot** those host pieces already call (`CURSOR-IHV-DRIVER-SPEC` §2). One-line split: Apple owns Metal.framework, WindowServer, AIR front-end, and IOGPU *framework*. We own firmware bring-up, PCI (or APU) match, framebuffer kext, accelerator kext, `*MTLDriver.bundle`, AIR → RDNA backend, present glue, and a power helper.

| Layer | HOST (do not rewrite) | AMD IHV creates |
|---|---|---|
| App GPU API | `Metal.framework` | Nothing. Apps keep linking Apple Metal. |
| Compositor | WindowServer / SkyLight | `IOFramebuffer` subclass WindowServer can open |
| Accelerator family | `IOAcceleratorFamily2` / `IOGPUFamily` (private SPI) | Vendor kext that **speaks** those user clients |
| Plugin discovery | Metal `dlopen`s `MetalPluginName` | Our `*MTLDriver.bundle`; registry keys on **our** accelerator |
| Shader front-end | `metal` / `metallib` → AIR | **AIR → gfx11 / gfx12 / gfx11.5 ISA** |
| First-party AMD | `AMDRadeonX6000*` / `*MTLDriver.bundle` | **Study ABI only. Do not ship, patch, or spoof.** |
| Compute-only dext | PCIDriverKit (TinyGPU’s shape) | Phase-1 **probe** only. Product is kexts. |

Trace the **live Tahoe Intel X6000 path** for IOGPU selectors and `MetalPluginClassName`. Do not invent selector numbers from iOS gists (`CURSOR-IHV-DRIVER-SPEC` §5).

---

## 3. Why 7000 before 9000 before Nvidia

1. **A live Metal ABI oracle exists, and it is AMD.** Official discrete Metal on current Intel macOS is `AMDRadeonX6000` + `AMDRadeonX6000Framebuffer` + `AMDRadeonX6000HWServices` for **Navi / RDNA1–2**. Official eGPU list ends at RX 6900 XT (device ID `0x73BF`) and RX 6600 XT ([102363](https://support.apple.com/en-us/102363); `01` §4.3; `03` §6.1). That stack is the thing to TRACE. Nvidia has no live Metal plugin on current macOS (Kepler fossils only; `03` §3.5).
2. **RDNA3 is one generation hop from that oracle**, not a foreign vendor. GPUOpen published the RDNA3 ISA ([GPUOpen RDNA3 ISA](https://gpuopen.com/news/rdna3-isa-guide-now-available/)). LLVM AMDGPU already names the chips (`gfx1100` = 7900 XTX/XT/GRE; `gfx1101` = 7800/7700 XT; `gfx1102` = 7600/7600 XT) ([LLVM AMDGPUUsage](https://llvm.org/docs/AMDGPUUsage.html)). `amdgpu` + RADV + ACO already speak the packets on Linux ([RADV](https://docs.mesa3d.org/drivers/radv.html)).
3. **TinyGPU already boots RDNA3+ as compute** (HIP / `comgr`, not Metal) via `setup_hipcomgr_osx.sh` ([TinyGPU](https://docs.tinygrad.org/tinygpu/)). Firmware-alive is a known class of problem, not a green field.
4. **RDNA4 (RX 9000) is a colder ISA.** GFX12 (`gfx1201` = 9070 / 9070 XT; `gfx1200` = 9060 / 9060 XT) launched commercially **6 Mar 2025** after a 28 Feb 2025 unveil ([AMD RDNA4 press](https://www.amd.com/en/newsroom/press-releases/2025-2-28-amd-unveils-next-generation-amd-rdna-4-architectu.html)). ISA docs exist ([RDNA4 ISA](https://docs.amd.com/v/u/en-US/rdna4-instruction-set-architecture); [GPUOpen on-shelf](https://gpuopen.com/learn/new_content_released_on_gpuopen_for_amd_rdna_4_on-shelf_day/)), but packets, DCN 4.x, and PSP 14 are farther from the X6000 oracle. Fill 7000 first; port the AIR backend and submit path.
5. **Nvidia is a different IHV slot** (`ihv/nvidia/`). No live Metal stack, GSP-era firmware, AIR → SASS from scratch. Do it after an AMD backend proves the host slot — or in parallel as a separate team. Do not block 7000 on Nvidia.

---

## 4. Hardware scope (all three backends)

### 4.1 RX 7000 dGPU — RDNA3 / Navi 3x — **first AMD target**

AMD’s 3 Nov 2022 unveil: first gaming GPU with a chiplet design. 5 nm GCD + 6 nm MCDs, Infinity Links up to 5.3 TB/s, Radiance Display Engine, DisplayPort 2.1 ([AMD RDNA3 press](https://www.amd.com/en/newsroom/press-releases/2022-11-3-amd-unveils-world-s-most-advanced-gaming-graphics-.html)).

| Product | LLVM `-mcpu` | Die | Notes |
|---|---|---|---|
| RX 7900 XTX | `gfx1100` | Navi 31 chiplet | 96 CU, 24 GB, 384-bit, 96 MB Infinity Cache; **1 GCD + 6 MCD** |
| RX 7900 XT | `gfx1100` | Navi 31 chiplet | 84 CU, 20 GB, 320-bit, 80 MB IC — **a disabled MCD**, not a different ISA |
| RX 7900 GRE | `gfx1100` | Navi 31 | Same GFX IP |
| RX 7800 XT / 7700 XT | `gfx1101` | Navi 32 chiplet | Smaller GCD + 4 MCD |
| RX 7600 / 7600 XT | `gfx1102` | Navi 33 **monolithic** | Prefer this SKU for **first** 7000 bring-up |

Host: Intel x86 Mac (2019 Mac Pro PCIe preferred) or Hackintosh x86. macOS 26 Tahoe pin (`CURSOR-IHV-DRIVER-SPEC` §4). Card’s own HDMI/DP for scanout. Thunderbolt needs `IOPCITunnelCompatible`. **2023 Mac Pro excluded** ([101988](https://support.apple.com/en-us/101988)).

Linux IP (living docs, not a port): Navi 31 is DCN **3.2.0**, GC **11.0.0**, VCN 4.0.0, SDMA 6.0.0, MP0/PSP **13.0.0** ([kernel amd-hardware-list](https://docs.kernel.org/gpu/amdgpu/amd-hardware-list-info.html)). Firmware names in linux-firmware: `psp_13_0_0_{sos,ta}.bin`, `smu_13_0_0.bin`, `gc_11_0_0_{pfp,me,mec,rlc,imu,mes*}.bin`, `dcn_3_2_0_dmcub.bin` ([linux-firmware Navi31](https://lists.ubuntu.com/archives/kernel-team/2022-December/135792.html)). macOS redistrib license = **UNKNOWN** until answered; linux-firmware ≠ a macOS grant.

### 4.2 Chiplet warning — 7900 is not a fat 6900 XT

The 7900 XTX is **one 5 nm Graphics Compute Die** (~306 mm², up to 96 CU) plus **six 6 nm Memory Cache Dies** (37.5 mm² each, 16 MB Infinity Cache + GDDR6 PHYs) ([AMD RDNA3 press](https://www.amd.com/en/newsroom/press-releases/2022-11-3-amd-unveils-world-s-most-advanced-gaming-graphics-.html)). Memory controllers and L3 live on the MCDs. Infinity Links are a new failure domain (training, harvest, disabled MCD on 7900 XT).

Do **not**: treat BAR/GMC/VM the way X6000 treats monolithic Navi 21; assume six healthy MCDs; skip SMU harvest reporting; start Phase 1 on a 7900 XTX if a 7600 is available. A first 7000 board of **Navi 33 / `gfx1102` / RX 7600** (monolithic, DCN 3.2.1, GC 11.0.2) is the sane bring-up. Keep 7900 XTX as the chiplet-hardening SKU after GCD-only compute works.

### 4.3 RX 9000 dGPU — RDNA4 / Navi 4x — **second**

Unveiled 28 Feb 2025; **on sale 6 Mar 2025** ([AMD RDNA4 press](https://www.amd.com/en/newsroom/press-releases/2025-2-28-amd-unveils-next-generation-amd-rdna-4-architectu.html); [GPUOpen on-shelf](https://gpuopen.com/learn/new_content_released_on_gpuopen_for_amd_rdna_4_on-shelf_day/)).

| Product | LLVM `-mcpu` | Notes |
|---|---|---|
| RX 9070 XT | `gfx1201` | 64 CU, 16 GB GDDR6, 256-bit, 64 MB IC, 304 W |
| RX 9070 | `gfx1201` | 56 CU, 16 GB, 220 W |
| RX 9060 / 9060 XT | `gfx1200` | Later SKUs; same GFX12 family |

There is **no shipping desktop RX 8000 dGPU**. AMD skipped that marketing generation; discrete RDNA4 is **RX 9000**. Do not invent an `ihv/amd-rdna4/` personality named “RX 8000”.

Linux firmware for Navi 48 names `dcn_4_0_1_dmcub.bin`, `gc_12_0_1_*`, `psp_14_0_3_*`, `smu_14_0_3.bin` (community packaging lists; confirm against [linux-firmware](https://gitlab.com/kernel-firmware/linux-firmware) for the exact ASIC). DCN 4.x and PSP 14 are **new IP** versus X6000 / Navi 31. Same host slot; new `ihv/amd-rdna4/{firmware,pci,display,submit,isa}/`.

### 4.4 8000-series iGPU — RDNA 3.5 APU — **third** (names as AMD actually uses them)

This is a **Hackintosh-on-AMD-APU** platform. Display is the APU’s DCN. Memory is **UMA** (Metal `Shared`, not discrete `Private`/`Managed`). It is **not** an Apple Silicon story and **not** a dGPU in a 2019 Mac Pro.

**As actually named by AMD (do not collapse these):**

| AMD name | Platform | Arch | LLVM | Display (kernel table) |
|---|---|---|---|---|
| **Radeon 800M series** (890M / 880M / 860M / 840M) | Ryzen AI 300 **Strix Point** | **RDNA 3.5** | `gfx1150` = 890M; `gfx1152` = 860M | DCN **3.5.0**, GC 11.5.0, MP0 14.0.0 |
| **Radeon 8060S / 8050S** (8000**S**, not 800M) | Ryzen AI Max 300 **Strix Halo** | RDNA 3.5, up to 40 CU | `gfx1151` = 8060S | DCN **3.5.1**, GC 11.5.1 |
| **Radeon 780M / 760M / 740M** (700M) | Ryzen 8040 **Hawk Point** | **RDNA 3**, not 3.5 | `gfx1103` class (Phoenix-family) | DCN **3.1.4**, GC 11.0.1 / 11.0.4 |

Sources: AMD Ryzen AI 300 “Radeon 800M Series” ([AMD how-to-sell](https://www.amd.com/content/dam/amd/en/documents/partner-hub/ryzen/amd-ryzen-ai-300-how-to-sell-guide-competitive.pdf); [AMD partner article](https://www.amd.com/en/partner/articles/ryzen-ai-300-series-processors.html)); LLVM product column ([AMDGPUUsage](https://llvm.org/docs/AMDGPUUsage.html)); kernel APU table ([apu-asic-info-table.csv](https://www.kernel.org/doc/Documentation/gpu/amdgpu/apu-asic-info-table.csv)).

**Hawk Point is not 800M and not RDNA 3.5.** Putting it in `ihv/amd-rdna35-igpu/` is a naming error. If a Hawk Point board is used, treat it as an RDNA3 APU (closer to Phoenix) or file it under a later `ihv/amd-rdna3-igpu/` — **UNKNOWN until Phase 0 names the exact APU**. First iGPU target = **Strix Point 890M**.

No `IOPCIDevice` in the Mac-Pro sense: the GPU is on the APU fabric. Matching is platform/ACPI-shaped, not a Thunderbolt dGPU personality. Exact IOKit nub on a Hackintosh AMD APU = **UNKNOWN** — measure; do not copy `IOPCIPrimaryMatch` from the 7000 Info.plist.

---

## 5. Study sources (read, do not port)

| Source | Use for | Do not |
|---|---|---|
| **Live `AMDRadeonX6000` on the same Tahoe build** | `MetalPluginName` / `MetalPluginClassName`, IOGPU `externalMethod` sizes, present, FB user client | Patch, spoof DID, ship Apple blobs |
| **GPUOpen ISA + machine-readable XML** | AIR → ISA: [RDNA3](https://gpuopen.com/news/rdna3-isa-guide-now-available/), [RDNA4](https://docs.amd.com/v/u/en-US/rdna4-instruction-set-architecture), [XML/IsaDecoder](https://gpuopen.com/machine-readable-isa/), [arch docs](https://gpuopen.com/amd-gpu-architecture-programming-documentation/) | Treat ISA PDF as a Metal plugin SDK |
| **LLVM AMDGPU backend** | Triple/`-mcpu`, address spaces, calling conv ([AMDGPUUsage](https://llvm.org/docs/AMDGPUUsage.html)) | Claim LLVM emits AIR or talks IOGPU |
| **`amdgpu` KMD** | IP bring-up: PSP, SMU, GC, DCN, GMC, SDMA. Living register docs | Load `amdgpu.ko` on XNU (`02` §7) |
| **RADV + ACO** | PM4 / CS shape, shader ABI; ACO is the default compiler ([RADV](https://docs.mesa3d.org/drivers/radv.html)) | Port DRM winsys or NIR into Metal.framework |
| **AMDVLK / PAL / LLPC** | Vendor-shaped pipeline ABI on Linux ([GPUOpen AMDVLK](https://gpuopen.com/amd-open-source-driver-for-vulkan/)) | Assume PAL exists on macOS |
| **TinyGPU AMD path** | Host-bootstrap PSP/SMU + HIP/`comgr` compute, **not Metal** ([TinyGPU](https://docs.tinygrad.org/tinygpu/); [tinygpu.md](https://github.com/tinygrad/tinygrad/blob/master/docs/tinygpu.md)) | Copy into `*MTLDriver.bundle`; call it display |
| **WhateverGreen / WhateverRed** | Negative examples: blob-enablement dies when the generation’s compiler is absent (`03` §4–5) | Copy patches |

---

## 6. Problem areas

None of these are stop-the-project. Attack as **research → trace → implement**. Success = Phase tests in §7.

| Area | Why hard | Read | Attack / OK |
|---|---|---|---|
| **Compiler (AIR → RDNA)** | Apps ship AIR inside `.metallib`. Apple’s AIRNT plugins only know `applegpu_g13*`… (`01` §5). LLVM AMDGPU / ACO emit **amdgcn**, not AIR. `CompilerPluginInterface` hook is **UNKNOWN** (`01` open q. 6). | WWDC 10615 / 10102; [AMDGPUUsage](https://llvm.org/docs/AMDGPUUsage.html); GPUOpen ISA | Offline AIR→`gfx1100` before any `MTLDevice` (Phase 4). If the MTLCompiler hook is sealed, compile inside `*MTLDriver.bundle` at PSO time. **OK:** trivial AIR compute runs on our submit path. |
| **Packets / CS** | X6000 packets are Apple-private. RDNA3 PM4 ≠ RDNA2. RADV records Linux CS, not IOGPU. | Live X6000 trace; RADV CS; TinyGPU AM/PM4 notes | Trace X6000 for **IOGPU shape**; emit **our** PM4/MES packets behind it. **OK:** Phase 3 identity fill; Phase 5 `makeLibrary` compute. |
| **PSP / SMU** | Signed firmware. PSP (MP0) loads SOS/TA; SMU clocks/thermals; GC needs PFP/ME/MEC/RLC/IMU/MES. Bad pointers brick the session (Asahi lesson). linux-firmware names are public; **macOS license is UNKNOWN**. | linux-firmware; `amdgpu` `psp_*` / `smu_*`; TinyGPU AMD | Boot PSP → SMU ready → GC firmware → doorbell no-op. Do not bit-bang 3D MMIO. Do not flash unsigned firmware. **OK:** P1 heartbeat. |
| **DCN (display)** | Display ≠ shader cores (`02` §4). X6000 FB is DCN 3.0-class (Navi 21). Navi 31 is DCN **3.2.0**; Strix Point DCN **3.5.0**; Navi 48 firmware is `dcn_4_0_1_*`. Radiance / DP 2.1 is new. | `IOFramebuffer.h`; `amdgpu` `dc/dcn32` / `dcn35` / `dcn401`; AMD Radiance notes | Dumb linear FB first (P2). Then hardware scanout. Do not reuse X6000 connector maps. **OK:** login window on the card. |
| **Chiplet (7900)** | GCD+MCD split. Harvested MCD on 7900 XT. GMC/VM/FB must see the real memory map. | AMD RDNA3 press; `amdgpu` Navi31 GMC | Prefer Navi 33 for P1–P4. On 7900, log MCD mask from SMU before first CS. **OK:** compute BO lands in real VRAM; no phantom 384-bit on a 5-MCD card. |
| **iGPU UMA** | No BAR VRAM. Metal storage modes flip (`01` §3.1). Display *is* the APU. Matching is not `IOPCIDevice`+TB. Hawk Point ≠ 800M. | `supportsFamily` storage-mode notes; kernel APU table | Separate `ihv/amd-rdna35-igpu/`. Honest `Shared` defaults. **OK:** FB on the APU outputs + compute on UMA without a dGPU personality. |

Shared host challenges (IOGPU ABI, WindowServer FB, no Graphics DriverKit, AIR drift) stay as in `CURSOR-IHV-DRIVER-SPEC` §10. Loading kexts is **not** the binding constraint; implementing the private contracts is.

---

## 7. Phased plan + tests (7000 first)

Do not skip phases. Stop-and-report if UNKNOWN blocks the success signal. Phases 0–6 run on **one** RDNA3 dGPU. Phase 7 is RDNA4. Phase 8 is RDNA 3.5 iGPU.

### Phase 0 — lock vendor + board

Freeze Intel x86 host, **one** Navi 3x DID (prefer `gfx1102` 7600), Tahoe build, bundle IDs (not `com.apple.*`). Snapshot working **X6000** IORegistry + `MTLCopyAllDevices` on the same OS (ABI oracle). List PSP/SMU/GC/DCN firmware license status.

**Accept:** P0.1 `boards.md` (host, GPU, VID/DID, BAR, connectors, OS build). P0.2 Intel x86; AS display excluded; 2023 Mac Pro excluded. P0.3 RDNA3 dGPU chosen; no RX 6000 spoof; 7900 chiplet called out if selected. P0.4 X6000 oracle captured. P0.5 UNKNOWN list filed.

### Phase 1 — enumerate + firmware alive

Personality attaches; BARs mapped; PSP SOS + SMU ready; one doorbell/RPC no-op. PCIDriverKit probe allowed, then retire.

**Accept:** P1.1 `IOPCIDevice` trained. P1.2 our `IOClass` on the nub. P1.3 VID/DID readable. P1.4 PSP/SMU/GC firmware ready **or** UNKNOWN + license block (no 3D bit-bang). P1.5 no-op completes. P1.6 still invisible to Metal.

### Phase 2 — dumb framebuffer

`IOFramebuffer` subclass; modeset; WindowServer desktop. No Metal.

**Accept:** P2.1 FB + console/panic. P2.2 login/desktop visible. P2.3 cursor/VBL stubbed. P2.4 unaccelerated OK.

### Phase 3 — non-Metal compute

BO alloc; PM4/MES compute packet; known pattern in a mapped buffer.

**Accept:** P3.1 alloc/map round-trip (on 7900: real MCD-backed VRAM). P3.2 submit + wait, bytes correct. P3.3 reset / TB yank safe. P3.4 still not Metal.

### Phase 4 — AIR backend (no MTLDevice)

`xcrun metal` → AIR → `gfx110x` via LLVM AMDGPU and/or ACO-shaped lowering. Run through Phase 3 submit.

**Accept:** P4.1 parse `.air`/`.metallib`; reject unknown triple. P4.2 emit ISA for trivial `kernel void add(...)`. P4.3 runs; buffer correct. P4.4 no `MTLCopyAllDevices` change.

### Phase 5 — MTLDriver.bundle

`MetalPluginName` on **our** accelerator. Tiny Metal compute app, normal `.metallib`. Honest `supportsFamily` (under-claim).

**Accept:** P5.1–P5.5 as host spec §11 Phase 5.

### Phase 6 — present

`CAMetalLayer` + `presentDrawable` on the Phase 2 FB. `CGDirectDisplayCopyCurrentMetalDevice` returns us.

**Accept:** P6.1–P6.4 as host spec.

### Phase 7 — RDNA4 (`ihv/amd-rdna4/`)

New VID/DID + PSP 14 / SMU 14 / GC 12 / DCN 4. AIR backend emits `gfx1201`. **Do not** rewrite `host/`. **Do not** spoof 7000 or 6000 IDs.

**Accept:** P7.1 personality only in `ihv/amd-rdna4/`. P7.2 that vendor’s firmware. P7.3 Phase 4 test ported. P7.4 `MTLCopyAllDevices` sees a **different** plugin name. P7.5 no 6000/7000 spoof.

### Phase 8 — RDNA 3.5 iGPU (`ihv/amd-rdna35-igpu/`)

Strix Point 890M (or named 800M). UMA + APU display. Hawk Point only if Phase 0 explicitly re-homes it.

**Accept:** P8.1 no dGPU DID table reuse. P8.2 FB on APU outputs. P8.3 compute on UMA. P8.4 Metal plugin distinct. P8.5 `host/` unchanged.

---

## 8. Repo layout (AMD fill)

```
macos-gpu-ihv/
  host/                          # IHV-agnostic — do not fork per ASIC
  ihv/amd-rdna3/                 # Phase 1 AMD fill — NOT X6000 spoof
    firmware/                    # PSP 13 / SMU 13 / GC 11.0 (blobs not in git unless license says so)
    pci/                         # Navi 3x VID/DID; tunnel flag
    display/                     # DCN 3.2.x + Radiance
    submit/                      # MES / PM4 / compute classes
    isa/                         # AIR → gfx1100/1101/1102
    chiplet/                     # MCD mask, harvest, GCD↔MCD map (7900)
  ihv/amd-rdna4/                 # Phase 7
    firmware/                    # PSP 14 / SMU 14 / GC 12
    pci/
    display/                     # DCN 4.0.1
    submit/
    isa/                         # AIR → gfx1200/1201
  ihv/amd-rdna35-igpu/           # Phase 8 — UMA APU, not a dGPU
    firmware/                    # PSP 14 / GC 11.5 / DCN 3.5
    match/                       # APU/platform match — not a 7000 DID list
    display/                     # APU DCN is the system display
    submit/
    isa/                         # AIR → gfx1150/1151/1152
    uma/                         # Shared-memory / IOSurface rules
```

`host/` must compile against a stub IHV. Adding RDNA4 must not edit RDNA3 ISA files. Nvidia stays in `ihv/nvidia/`.

---

## 9. Do-nots

1. **Do not spoof an RX 6000 / X6000 personality onto 7000, 9000, or 800M.** That is WhateverGreen-class blob enablement and cannot create a generation Apple did not ship (`03` §5.1; WhateverRed).
2. **Do not call Hawk Point “800M” or “RDNA 3.5”.** AMD names it Radeon 700M / RDNA 3.
3. **Do not invent a desktop RX 8000 dGPU backend.** Discrete RDNA4 is RX 9000.
4. **Do not write SIP, OpenCore, AuxKC, 1TR, or unsigned-kext steps.** If a kext does not attach, collect IORegistry and stop.
5. **Do not claim `amdgpu.ko` / RADV / TinyGPU HIP will load as Metal.** Hardware notes transfer; OS integration does not (`02` §7).
6. **Do not replace Metal.framework, IOGPU.framework, WindowServer, or the AIR front-end.**
7. **Do not start display/Metal on Apple Silicon AGX/DCP.** AGX stays system GPU.
8. **Do not treat 2023 Mac Pro PCIe as a GPU host** ([101988](https://support.apple.com/en-us/101988)).
9. **Do not flash unsigned PSP/SMU/GC firmware.** License UNKNOWN → stop.
10. **Do not invent IOGPU selectors, `MetalPluginClassName` vtables, or AIR opcodes.** UNKNOWN + cite.
11. **Never say impossible.** Say what is hard, why, and which phase attacks it.

---

## 10. Open questions

Resolve from public headers, the pinned Tahoe KDK, AMD docs, and traces. Do not guess.

1. Exact `Metal.framework` ↔ `*MTLDriver.bundle` vtable on macOS 26 Intel (shared with host §14).
2. Tahoe Intel: IOGPU user clients vs `IOAccelContext2` for discrete AMD — measure X6000 (`01` open q. 3).
3. Can `CompilerPluginInterface` register a non-Apple AIR backend, or must compilation live inside `*MTLDriver.bundle`?
4. Confirmation no Apple **X7000 / X8000 / X9000** Metal kext appeared after the 27 Aug 2026 research dump.
5. macOS-redistributable license for `psp_13_*` / `smu_13_*` / `gc_11_*` / `dcn_3_2_*` (and RDNA4/3.5 siblings).
6. First 7000 DID: Navi 33 vs Navi 31. If Navi 31, required MCD-harvest bring-up steps = UNKNOWN until SMU traces.
7. Whether X6000 DCN user-client methods are close enough to DCN 3.2 that P2 can stub, or a full new FB protocol is required — measure, do not assume.
8. Cross-device IOSurface without CPU (`01` open q. 8) — discrete 7000/9000 vs UMA 800M are different answers.
9. APU match: what nub a Strix Point iGPU publishes on a Hackintosh x86 firmware (ACPI name vs PCI). UNKNOWN.
10. Honest minimum `MTLGPUFamily` for gfx11 compute-only vs render.
11. TinyGPU AMD: how much of PSP/SMU/GC it actually boots vs a compute subset (`03` open q. 4 analog).
12. RDNA4 `gfx1201` packet deltas vs `gfx1100` that break a naive PM4 port — list from RADV/ACO before Phase 7.

---

## 11. Sources

**On disk:** `CURSOR-IHV-DRIVER-SPEC.md`, `01-metal-userspace.md`, `02-kernel-boot-display.md`, `03-nvidia-prior-art.md`, `FINDINGS.md`.

**Apple (last discrete Metal / policy):** [102363](https://support.apple.com/en-us/102363) (eGPU through 6900 XT `0x73BF` / 6600 XT; aftermarket drivers; Intel required); [101988](https://support.apple.com/en-us/101988); [101641](https://support.apple.com/en-us/101641); [IOFramebuffer.h](https://github.com/apple-oss-distributions/IOGraphics/blob/main/IOGraphicsFamily/IOKit/graphics/IOFramebuffer.h); [philipturner MetalPluginName](https://gist.github.com/philipturner/48c72e3fcce0ce9489071eb083a5086e).

**AMD product / naming:** [RDNA3 / 7900 XTX chiplet](https://www.amd.com/en/newsroom/press-releases/2022-11-3-amd-unveils-world-s-most-advanced-gaming-graphics-.html); [RDNA4 / 9070 / 9070 XT, 28 Feb 2025, sale 6 Mar 2025](https://www.amd.com/en/newsroom/press-releases/2025-2-28-amd-unveils-next-generation-amd-rdna-4-architectu.html); [Ryzen AI 300 / Radeon 800M](https://www.amd.com/en/partner/articles/ryzen-ai-300-series-processors.html); [800M series sell guide](https://www.amd.com/content/dam/amd/en/documents/partner-hub/ryzen/amd-ryzen-ai-300-how-to-sell-guide-competitive.pdf).

**Compiler / packets / firmware:** [LLVM AMDGPUUsage](https://llvm.org/docs/AMDGPUUsage.html); [GPUOpen ISA index](https://gpuopen.com/amd-gpu-architecture-programming-documentation/); [RDNA3 ISA news](https://gpuopen.com/news/rdna3-isa-guide-now-available/); [RDNA4 ISA](https://docs.amd.com/v/u/en-US/rdna4-instruction-set-architecture); [machine-readable ISA](https://gpuopen.com/machine-readable-isa/); [RADV](https://docs.mesa3d.org/drivers/radv.html); [AMDVLK](https://gpuopen.com/amd-open-source-driver-for-vulkan/); [kernel AMD hardware list](https://docs.kernel.org/gpu/amdgpu/amd-hardware-list-info.html); [APU ASIC table](https://www.kernel.org/doc/Documentation/gpu/amdgpu/apu-asic-info-table.csv); [linux-firmware Navi31](https://lists.ubuntu.com/archives/kernel-team/2022-December/135792.html); [linux-firmware repo](https://gitlab.com/kernel-firmware/linux-firmware).

**TinyGPU (compute, not Metal):** [docs](https://docs.tinygrad.org/tinygpu/); [tinygpu.md](https://github.com/tinygrad/tinygrad/blob/master/docs/tinygpu.md); [heise](https://www.heise.de/en/news/First-eGPU-drivers-for-Apple-Silicon-Macs-but-only-for-AI-11246984.html).

**Negative / methodology:** [WhateverRed](https://github.com/ainexur/WhateverRed); [WhateverGreen](https://github.com/acidanthera/WhateverGreen/blob/master/README.md); [Asahi AGX](https://asahilinux.org/docs/hw/soc/agx/).

---

*Architecture and implementation plan only. Loading kexts is out of scope. If a claim is not cited, treat it as UNKNOWN and look it up before coding.*
