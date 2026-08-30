# CURSOR-START-INTEL — IHV slot for Intel Arc dGPU + modern Intel iGPU

**Audience:** Cursor coding agents filling the Intel IHV slot.  
**Host contract (binding; do not rewrite):** `CURSOR-IHV-DRIVER-SPEC.md`. This file is the **vendor appendix** for Intel. Implement `ihv/arc/` and `ihv/intel-igpu/` behind the same host doors. Do **not** invent a second Metal stack.  
**Research corpus (cite; do not invent APIs):** `01-metal-userspace.md`, `02-kernel-boot-display.md`, `03-nvidia-prior-art.md`, `FINDINGS.md`.  
**Date:** 29 Aug 2026. Corpus date: 27 Aug 2026.  
**Hard rule:** architecture and implementation plan only. No SIP/OpenCore/unsigned-kext recipes, no kext-patch how-tos, no exploits. Loading is **assumed solved** on Intel x86 Hackintosh / Intel Mac. If attach fails: collect IORegistry and stop.

If a string, selector, entitlement, bundle ID, IOGPU method, GuC CTB opcode, or Xe ISA encoding is not in the host spec, the three research files, or a **public** Apple/Intel/kernel header you have actually opened, write **UNKNOWN** and stop that branch.

---

## 1. Goal

Build the **Intel IHV backends** so a GPU Apple never shipped as a Metal device — **Intel Arc (discrete)** and/or a **modern Intel iGPU (Xe-LPG / Xe2 and later)** — can do **display scanout and Metal** on a machine where kexts can be loaded for development (Intel Mac / Hackintosh x86).

This is the host’s **Phase 7** slot (`CURSOR-IHV` §9.2, §11 Phase 7): Nvidia fills first; Arc/iGPU plug in **without** rewriting matching, framebuffer shell, IOGPU glue, present, or the `*MTLDriver.bundle` skeleton. Swap firmware, PCI IDs, ISA backend, and display-engine programming.

**Done** is the host definition (`CURSOR-IHV` §3): WindowServer attaches to **our** `IOFramebuffer`; `MTLCopyAllDevices()` returns **our** GPU; a stock `.metallib` compute app runs on **Xe ISA**; a Metal render app presents via `CAMetalLayer` and `CGDirectDisplayCopyCurrentMetalDevice` returns **our** device.

**Stance:** ridiculously difficult — private host contracts + signed GuC/HuC + a new AIR→Xe compiler + two different display/memory models. **Not impossible.** Do not say impossible. Do not treat WhateverGreen / OCLP “Intel is dead after Ice Lake” as a product rule; those projects **enable old AppleIntel\* blobs**. We write a **new IHV backend**. Apple’s policy that aftermarket GPU drivers are “not compatible with macOS” is a **product rule**, not a missing header ([102363](https://support.apple.com/en-us/102363)).

TinyGPU proves Apple will talk to foreign GPUs as **compute** coprocessors. It is **not** this product (no `MTLDevice`, no WindowServer). Apple Silicon is **irrelevant** here: AS iGPU is **AGX**, not Xe. Do not confuse them (`02` §1.2; `FINDINGS` §3).

---

## 2. Host vs IHV — what you implement

Apple owns the API, compositor, AIR front-end, and IOGPU *framework*. We own firmware bring-up, PCI match, framebuffer kext, accelerator kext, Metal plugin, AIR→Xe backend, present glue, and a power helper. **Copy this table from the host; do not invent extra host APIs.**

| Layer | HOST (Apple ships — do not rewrite) | IHV (this slot creates) |
|---|---|---|
| App GPU API | `Metal.framework` (`MTLDevice`, queues, `MTLCopyAllDevices`, `supportsFamily`) | Nothing. Apps keep linking Apple Metal. |
| Drawables / present | `QuartzCore` / `CAMetalLayer`; `CGDirectDisplayCopyCurrentMetalDevice` | IOSurface-backed present WindowServer will composite |
| Compositor | WindowServer / SkyLight | `IOFramebuffer` **user client** (kext subclass) |
| Cross-process pixels | `IOSurface.framework` + kernel IOSurface | GPU-map IOSurface BOs on **this** device (VRAM or UMA) |
| Display family | `IOGraphicsFamily.kext` — `IOFramebuffer` class ([`IOFramebuffer.h`](https://github.com/apple-oss-distributions/IOGraphics/blob/main/IOGraphicsFamily/IOKit/graphics/IOFramebuffer.h)) | Subclass for **this** card’s / package’s display engine |
| Accelerator family | `IOAcceleratorFamily2.kext`, `IOGPUFamily.kext` (private SPI) | Vendor accelerator that **speaks** those user clients |
| Plugin discovery | Metal `dlopen`s `MetalPluginName` / `MetalPluginClassName` ([philipturner gist](https://gist.github.com/philipturner/48c72e3fcce0ce9489071eb083a5086e)) | Our `*MTLDriver.bundle`; registry keys on **our** accelerator |
| Shader front-end | `metal` / `metallib` → AIR; `MTLCompilerService` ([WWDC20 10615](https://developer.apple.com/videos/play/wwdc2020/10615/)) | **AIR → Xe ISA** backend. We ingest AIR; we do not write MSL→AIR |
| PCI / IOKit | `IOPCIFamily`, matching, TB `IOPCITunnelCompatible` | Personalities: VID `8086`, DID allow-list; tunnel flag if TB |
| Power | `AppleGraphicsPowerManagement`, `AppleGPUWrangler`, AGDC — **private** | Helper: clocks, thermal, surprise-remove. **No AGPM injector** |
| First-party Intel | `AppleIntel*Graphics` / `*MTLDriver.bundle` (Gen9/11 only — §4.3) | **Study ABI only. Do not ship, patch, or spoof.** |

**One-line split:** host contracts in `CURSOR-IHV` §5 are **identical** for Nvidia, Arc, and modern iGPU. Vendor code lives in `ihv/arc/` and `ihv/intel-igpu/`.

**Kext vs dext:** FB + Metal are **kexts**. No Graphics DriverKit family (`02` §3; [WWDC19-702](https://developer.apple.com/videos/play/wwdc2019/702/)). PCIDriverKit is a Phase 1 **BAR/firmware probe only**, then retire it.

**Platform:** Intel x86 Mac or Hackintosh x86; develop against **macOS 26 Tahoe** (last major Intel macOS). Preferred Arc host: **2019 Mac Pro (MacPro7,1) PCIe** or Intel TB3 enclosure. 2023 Mac Pro is **not** a graphics-card host ([101988](https://support.apple.com/en-us/101988)). 2019 Mac Pro has **no iGPU** ([101641](https://support.apple.com/en-us/101641)) — Arc must drive **its own** HDMI/DP. Modern iGPU on a Hackintosh **is** the boot display (package display engine + GOP).

---

## 3. Hardware scope — two backends, one host

Both backends share host contracts. They **do not** share firmware blobs, display programming, or memory model. Pick **one** first board in Phase 0; do not dual-bring-up.

### 3.1 Arc dGPU (discrete — `ihv/arc/`)

**Never a Mac IHV.** No Apple personality, no `*MTLDriver.bundle`, no compiler plugin (`CURSOR-IHV` §9.2). Community enablement treats all Arc as unsupported because Apple never wrote a driver ([Dortania Intel GPU guide](https://dortania.github.io/GPU-Buyers-Guide/modern-gpus/intel-gpu.html)). That is blob-absence, not a physics limit. An A770 in a 2019 Mac Pro enumerates as a generic adapter and does not scan out — expected with no IHV ([Gant, Apr 2025](http://blog.greggant.com/posts/2025/04/20/intel-arc-a770-mac-pro-2019-bad-idea.html)).

| Family | Arch / code | What actually shipped (public, 2026) | Memory / bus | Display |
|---|---|---|---|---|
| **Alchemist A-series** | Xe-HPG, DG2, ACM-G10/G11/G12, TSMC N6 | Desktop A310/A380/A580/A750/A770 (2022–23); mobile A350M–A770M; Pro A30M/A40/A50/A60 | Dedicated **GDDR6 VRAM**; PCIe 4.0 | **Own** display engine (Xe-HPD). Card HDMI/DP. |
| **Battlemage B-series** | Xe2-HPG, BMG-G21 then BMG-G31 | Gaming B580 (Dec 2024), B570 (Jan 2025); Pro B50/B60 (2025); Pro **B70 / B65** Q1’26 (32 Xe-cores / 32 GB on B70, DID `0xE223`) ([Intel Arc Pro B70](https://www.intel.com/content/www/us/en/products/sku/245797/intel-arc-pro-b70-graphics/specifications.html); [Wikipedia: Intel Xe](https://en.wikipedia.org/wiki/Intel_Xe)) | Dedicated **GDDR6 VRAM**; B70 PCIe **5.0 x16** | Own DE; DP 2.1 UHBR 13.5 on B70 |

**PCI:** vendor `0x8086`. Example DIDs (confirm on the board with `system_profiler` / IORegistry — do not guess a full table): A770 `0x56A0` ([ACM PRM vol. 4](https://kiwitree.net/~lina/intel-gfx-docs/prm/acm/intel-gfx-prm-osrc-acm-vol04-configurations.pdf)); B70 `0xE223`, B65 `0xE222` ([Intel ARK B70](https://www.intel.com/content/www/us/en/products/sku/245797/intel-arc-pro-b70-graphics/specifications.html) / [B65](https://www.intel.com/content/www/us/en/products/sku/245796/intel-arc-pro-b65-graphics/specifications.html)). Full allow-list is a Phase 0 deliverable.

**Phase 0 default (recommended):** one **Alchemist** board first (A750/A770) — older GuC pairing, more public PRM/i915 text. Battlemage is in-scope as the same backend with a **different** GuC/HuC pair and Xe2 ISA variant. Do not mix A-series and B-series firmware.

**Out of product scope:** Ponte Vecchio / Max / Flex datacenter; DG1 Iris Xe MAX (Xe-LP discrete, 2020–21, not Arc branding as shipped retail); Celestial/Xe3P “next Arc” until a board exists in the lab.

### 3.2 Modern iGPU (UMA — `ihv/intel-igpu/`)

**This is not Apple’s last Intel Mac iGPU** (that is Gen9/11 — §4.3). Target silicon Apple **never** shipped:

| CPU package (as sold by 2026) | iGPU arch | Marketing names | Notes |
|---|---|---|---|
| **Meteor Lake** — Core Ultra 100 | **Xe-LPG** (Alchemist-derived, tile GPU) | “Intel Graphics” / “Intel Arc Graphics” on H-series | UMA; display on SoC/package. Linux default KMD = **i915** ([Phoronix MTL i915 vs Xe](https://www.phoronix.com/review/intel-mtl-i915-xe-linux)). |
| **Arrow Lake** — Core Ultra 200S / 200H / 200HX | **Xe-LPG** (H has more Xe-cores + XMX; HX often 4 Xe-cores, no XMX) | Arc 130T / 140T on H; “Intel Graphics” on S/HX | Still Xe-LPG, **not** Xe2, despite 200-series branding ([Wikipedia: Intel Xe](https://en.wikipedia.org/wiki/Intel_Xe)). |
| **Lunar Lake** — Core Ultra 200V | **Xe2-LPG** | Arc 130V / 140V | First Xe2 iGPU (Sep 2024). Linux default KMD = **Xe**. |
| **Panther Lake** — Core Ultra 300 | **Xe3** | Sold 2026 | Newer ISA revision. Treat as **stretch**; freeze only if that is the only board. |

Also sold, **not** a first board: Xe-LP iGPU on Tiger/Alder/Raptor Lake (Iris Xe / UHD). Same “new IHV” rule (Apple never shipped Gen12), but older than LPG. Record as optional later SKU.

**iGPU facts that change the driver:**

- **UMA**, not VRAM. Metal storage is Shared-like; `Managed`/`Private` discrete rules do not apply the same way (`01` §3.1).
- Display engine is **on the CPU package**. On a Hackintosh this **is** the boot display (EFI GOP → our `IOFramebuffer`). On a real Intel Mac with a supported AMD/iGPU already driving the panel, a *second* modern iGPU does not exist — the host iGPU **is** Gen9/11.
- PCI function is typically `00:02.0` (IGD), not a slot card. Still `IOPCIDevice` matching (`02` §2.1).
- Stolen / DSM / stolen-size are **Linux i915 concepts**. Whether Tahoe WindowServer needs an analog is **UNKNOWN** — measure; do not invent `AAPL,ig-platform-id` (that is WhateverGreen, not this project).

### 3.3 What is not in scope

- **Apple Silicon iGPU (AGX).** Different IP, ADT `gpu,t*`, DCP scanout. Do not read Asahi AGX as Intel bring-up (`02` §1.2).
- **Spoofing `AppleIntelKBL*` / `AppleIntelICL*` onto Xe-LPG or Arc.** Those kexts are Gen9/11. WhateverGreen itself sets `gPlatformGraphicsSupported = false` at Rocket Lake+ ([`kern_igfx.cpp`](https://github.com/acidanthera/WhateverGreen/blob/008bfc3129e8e196beb44238437f325f2dd33e9d/WhateverGreen/kern_igfx.cpp)). Same lesson as WhateverRed on unshipped AMD: you cannot invent purged logic (`03` §5.1).
- **Iris / UHD 630 as a *product* target.** Trace-only (§4.3).

---

## 4. Study sources (hardware knowledge; OS integration does not transfer)

Linux DRM is the **wrong kernel** (`02` §7). Read it for MMIO, GuC boot, command packets, display clocks. Re-express as IOKit + Metal. **Do not port `i915.ko` / `xe.ko`.**

### 4.1 i915 vs Xe DRM

| Driver | Role (public kernel docs / Intel) | Default platforms |
|---|---|---|
| **`drm/i915`** | Production KMD for integrated GFX through **Meteor Lake** and discrete **DG2/Alchemist** | TGL…MTL, DG2. Xe on these is `force_probe` / experimental forever for ABI stability ([Xe RFC](https://docs.kernel.org/next/gpu/rfc/xe.html); [compute-runtime #811](https://github.com/intel/compute-runtime/issues/811)). |
| **`drm/xe`** | New KMD (TTM, drm-scheduler, gpuvm). **Official** from **Lunar Lake + Battlemage** | LNL, BMG, and newer. Display **shared** with i915 ([drm/xe index](https://docs.kernel.org/gpu/xe/index.html); [Phoronix Battlemage display](https://www.phoronix.com/news/Intel-Linux-Display-Battlemage)). |

**Read:** `drivers/gpu/drm/i915/` (GuC, execbuf, DG2), `drivers/gpu/drm/xe/` (CTB, SLPC, LNL/BMG), shared `display/` (CDCLK, pipes, DMC). Mesa **ANV** / Iris and **Intel Graphics Compiler (IGC)** / compute-runtime for ISA **shape** ([IGC FOSDEM 2024](https://archive.fosdem.org/2024/events/attachments/fosdem-2024-2501-challenges-of-supporting-multiple-versions-of-llvm-in-intel-graphics-compiler/slides/22869/fosdem2024-llvm-in-igc_KVhVGa2.pdf); [IGC 2.34.4](https://www.phoronix.com/news/Intel-IGC-2.34.4)). IGC: LLVM → **VISA** → custom emitter — **not** an AIR backend and **not** a Mesa drop-in to Metal.

Intel **PRMs** (Alchemist volumes, etc.) are the closest public register bible. Confirm license before copying tables into this tree.

### 4.2 GuC / HuC / GSC / DMC

Modern Intel submission is **firmware-brokered**, same thesis as Nvidia GSP / Asahi AGX: boot the microcontroller, speak its RPC, do **not** bit-bang shader-core MMIO ([kernel Xe firmware](https://docs.kernel.org/gpu/xe/xe_firmware.html)).

| Firmware | Job | Layout / pairing (public) |
|---|---|---|
| **GuC** | Scheduling, submission, SLPC power, CTB H2G/G2H | CSS header + uCode + RSA on **all** platforms. Blobs versioned (e.g. `*_guc_70.bin`). WOPCM carved for GuC/HuC. |
| **HuC** | Media (clear + protected) | CSS through DG1; **GSC layout from DG2/MTL**. MTL+: 2-step auth (GuC then GSC) ([Xe firmware doc](https://docs.kernel.org/gpu/xe/xe_firmware.html)). |
| **GSC** | Security / HuC load on DG2+ | Directory + CPD; DG2 HuC load via MEI — **macOS MEI analog is UNKNOWN**. |
| **DMC** | Display microcontroller | Separate blob (`mtl_dmc.bin`, `bmg_dmc.bin`, …). Display ≠ render. |

Example linux-firmware names (study; **not** a macOS grant): `i915/dg2_guc_70.bin`, `i915/dg2_huc_gsc.bin`, `i915/mtl_guc_70.bin`, `i915/mtl_huc_gsc.bin`, `i915/mtl_gsc_1.bin`, `xe/bmg_guc_70.bin`, `xe/bmg_huc.bin`, `xe/lnl_guc_70.bin`, `xe/lnl_huc.bin` ([intel-xe firmware PR, Aug 2024](https://lists.freedesktop.org/archives/intel-xe/2024-August/044362.html)). **GuC major must pair with the platform.** A TGL GuC on BMG is a brick.

**GuC CTB:** single blob, H2G/G2H descriptors + buffers, 4K multiples ([Xe firmware](https://docs.kernel.org/gpu/xe/xe_firmware.html)). That is the doorbell analog for Phase 1.

**License:** linux-firmware ≠ macOS redistributable grant (`CURSOR-IHV` open q. 8). Record UNKNOWN; do not flash unsigned firmware; do not commit blobs unless the license says so.

### 4.3 Apple’s last Intel Mac iGPU — TRACE REFERENCE ONLY

Apple shipped Intel iGPU Metal through **Ice Lake / Comet Lake-era** parts. That stack is **Gen9 / Gen11**, not Xe.

| Apple generation | Accelerator / FB kext | Metal bundle (examples) | Silicon |
|---|---|---|---|
| Kaby / Coffee / Comet Lake | `AppleIntelKBLGraphics` + `AppleIntelKBLGraphicsFramebuffer` / `AppleIntelCFLGraphicsFramebuffer` | `AppleIntelKBLGraphicsMTLDriver.bundle` | HD/UHD 610–655, **UHD 630** |
| Ice Lake | `AppleIntelICLGraphics` + `AppleIntelICLLPGraphicsFramebuffer` | ICL MTL sibling (`01` §4.4) | **Iris Plus** G4/G7 |

Inventories: `01` §4.4; [Mojave SLE gist](https://gist.github.com/knightsc/6c188ad9c065d7c134de88564974952c); [WhateverGreen `kern_igfx_kexts.cpp`](https://github.com/acidanthera/WhateverGreen/blob/bc1b7c33/WhateverGreen/kern_igfx_kexts.cpp).

**Last Intel Macs that still matter on Tahoe** (Intel cutoff after 26; Golden Gate is AS-only — [MacRumors](https://www.macrumors.com/2026/08/20/macos-golden-gate-marks-the-end-of-an-era/)):

- 13″ MacBook Pro 2020 (4× TB3): Ice Lake **Iris Plus** ([Apple 111339](https://support.apple.com/en-euro/111339)).
- 16″ MacBook Pro 2019: UHD 630 **+ AMD dGPU**.
- 27″ iMac 2020: Comet Lake; **display is AMD Radeon Pro 5000**, not the UHD 630 ([Apple 111913](https://support.apple.com/en-us/111913)).
- 2019 Mac Pro: **no iGPU**.

**Use this stack to:** dump IORegistry (`MetalPluginName`, accelerator class, FB user client) on the **same Tahoe build** as the ABI oracle **if** you have a KBL/ICL Intel Mac. Prefer the host’s **AMD X6000** oracle when both exist (`CURSOR-IHV` §11 P0.4) — AMD is the discrete IHV reference; Intel Gen9/11 is the *iGPU-shaped* reference (UMA, IGD `00:02.0`, FB+accel split).

**Do not use this stack to:** copy Gen9/11 command packets, spoof ICL/KBL IDs onto MTL/ARL/LNL/Arc, or claim `AppleIntelKBLGraphicsMTLDriver` will JIT Xe. AIR→EU (Gen9/11) ≠ AIR→Xe-core. Apple AIRNT plugins only name `applegpu_g13*`…`g18p` ([zboralski gist](https://gist.github.com/zboralski/524d292e8c1fa5cb64500a85874a333b)) — **no** Intel target.

---

## 5. Problem areas (first-class; attack, do not mythologize)

Host challenges in `CURSOR-IHV` §10 all apply (IOGPU ABI, AIR hook, WindowServer FB, no Graphics DriverKit, per-OS drift). Intel-specific poles:

| Challenge | Why hard | Attack / success |
|---|---|---|
| **GuC (not GSP)** | Submission and PM live in GuC. Wrong blob / WOPCM / CTB = dead GT or hang. HuC/GSC needed for media, not for first compute. DG2 HuC via MEI has **no public macOS path**. | **R:** Xe firmware doc + i915/xe GuC load. **T:** BAR map → CSS parse → DMA load → CTB no-op. **I:** Phase 1 heartbeat only; SLPC later. **OK:** logged GuC ready + one H2G/G2H round-trip. Do not poke 3D MMIO. |
| **Xe ISA vs Apple’s last Intel AIR backend** | Apple’s Intel Metal compiler is Gen9/11 EU. Xe/Xe2/Xe3 is a **new ISA** (Xe-core, vector + XMX). IGC/ANV/NIR know Xe; they speak SPIR-V/NIR, not AIR. No public AIR→Xe. | Same Asahi lesson: **compiler before API**. Phase 4: `xcrun metal` trivial AIR → Xe binary → Phase 3 submit. Study IGC VISA / Mesa intel compiler as **ISA notes**. If `CompilerPluginInterface` is sealed, compile inside `*MTLDriver.bundle` (`CURSOR-IHV` §7.6). **OK:** P4 add-one kernel on our ISA. |
| **Display engine ≠ shader cores** | Arc: discrete DE + CDCLK + DMC + own connectors (DG2 CDCLK table is public in i915). iGPU: package DE, often **the** GOP boot FB; CDCLK/stolen/DMC differ by PCH/SoC. Shared Linux display code is KMS, not `IOFramebuffer`. | Phase 2 dumb linear FB first (host §11). Then modeset/cursor/VBL/hot-plug. **OK:** P2 login window. Do not block FB on AIR. |
| **iGPU vs dGPU memory + boot** | Arc: VRAM, BAR2/ReBAR (2019 Mac Pro ReBAR **not** on by default — [Gant](http://blog.greggant.com/posts/2025/04/20/intel-arc-a770-mac-pro-2019-bad-idea.html); treat ReBAR as UNKNOWN/optional). iGPU: UMA, IGD, boot display, possibly stolen memory. IOSurface mapping differs. | Separate `ihv/*/display` and `ihv/*/submit` memory managers. Honest `supportsFamily` / storage-mode. **OK:** P3 buffer round-trip on the memory model you actually have. |
| **Two GT / media + render** | Xe-LPG+ often splits media GT vs render GT; HuC sits on media. | Phase 1–4 **render/compute GT only**. Media/VA is later. |

---

## 6. Phased plan + acceptance tests

Do not skip phases. Do not start a Metal plugin on day one. Stop-and-report if UNKNOWN blocks the signal. Host Phase 0–6 tests apply; Intel deltas below. **This file is Phase 7 work** relative to Nvidia — still run P0–P6 on the Intel board.

### Phase 0 — lock vendor + board

**Work:** pick **either** one Arc DID **or** one modern iGPU package (not both). Pin Tahoe build. Snapshot AMD (and, if present, AppleIntel KBL/ICL) IORegistry on the same OS. List firmware license. Read this file + host §5/§11 + `01`–`03`.

| ID | Criterion |
|---|---|
| I0.1 | `boards.md`: host model, GPU name, VID/DID, BAR sizes, connectors, OS build, **Arc vs iGPU** |
| I0.2 | Intel x86 host; AS display excluded; 2023 Mac Pro excluded |
| I0.3 | Target is Xe-LPG+ or Arc — **not** UHD 630 / Iris Plus as product silicon |
| I0.4 | No `AppleIntel*` personality spoof in the plan |
| I0.5 | UNKNOWN list: GuC redistrib, MEI/GSC, IOGPU selectors, ReBAR, stolen-memory |

### Phase 1 — enumerate + GuC alive

Personality attaches; BARs mapped; **GuC** (not 3D) heartbeats; one CTB no-op.

| ID | Criterion |
|---|---|
| I1.1–I1.3 | Host P1.1–P1.3 (nub, `start`, VID/DID) for `8086` + frozen DID |
| I1.4 | Platform-correct GuC reaches logged ready **or** UNKNOWN + license block |
| I1.5 | One H2G/G2H or identity fill; no panic |
| I1.6 | Still invisible to `MTLCopyAllDevices` |

### Phase 2 — dumb framebuffer

`IOFramebuffer` subclass. Arc: modeset on **card** ports. iGPU: claim the **package** scanout / GOP handoff.

| ID | Criterion |
|---|---|
| I2.1–I2.4 | Host P2.1–P2.4 (console, WindowServer login, cursor/VBL stub, no Metal) |

If WindowServer refuses a dumb FB: capture the user-client error; implement the **minimum** extra methods; do not “fix” display with Metal.

### Phase 3 — non-Metal compute

BO alloc, GuC-submitted compute packet, known pattern in a mapped buffer.

| ID | Criterion |
|---|---|
| I3.1–I3.4 | Host P3 (map round-trip, submit+wait, reset / TB yank, still not Metal) |

### Phase 4 — AIR → Xe (not yet MTLDevice)

Trivial AIR compute (`kernel void add(device uint* p [[buffer(0)]]) { p[0] += 1; }`) → Xe ISA → Phase 3 path.

| ID | Criterion |
|---|---|
| I4.1–I4.4 | Host P4. Reject unknown `air64` triple. No `MTLCopyAllDevices` change. |

### Phase 5 — `*MTLDriver.bundle` + `MetalPluginName`

| ID | Criterion |
|---|---|
| I5.1–I5.5 | Host P5. Plugin name **ours**, not `AppleIntel*MTLDriver`. Honest `supportsFamily` (under-claim). |

### Phase 6 — present

| ID | Criterion |
|---|---|
| I6.1–I6.4 | Host P6. Arc: VRAM IOSurface. iGPU: UMA IOSurface. `CGDirectDisplayCopyCurrentMetalDevice` = us. |

### Phase 7 — second Intel backend

If Arc shipped first, add iGPU (or reverse) **without** editing `host/` or the other IHV’s ISA files.

| ID | Criterion |
|---|---|
| I7.1 | New VID/DID only under the **other** `ihv/*` |
| I7.2 | That backend’s GuC/HuC pair, not a copy of the first |
| I7.3 | AIR backend emits that ISA; P4 ported |
| I7.4 | Different `MetalPluginName` |
| I7.5 | No KBL/ICL spoof; no `i915.kext` pretence |

---

## 7. Repo layout (Intel fill)

Host layout is `CURSOR-IHV` §12. Intel adds two IHV trees. `host/` must compile against a stub IHV.

```
macos-gpu-ihv/
  host/                          # unchanged by this slot
  ihv/arc/                       # discrete Xe-HPG / Xe2-HPG
    firmware/                    # GuC/HuC/GSC/DMC (blobs not in git unless licensed)
    pci/                         # 8086 + A-series / B-series DID tables
    display/                     # Arc DE, CDCLK, card connectors, VRAM scanout
    submit/                      # GuC CTB / compute classes
    isa/                         # AIR → Xe (Alchemist / Battlemage variants)
  ihv/intel-igpu/                # UMA Xe-LPG / Xe2-LPG / later
    firmware/                    # MTL/ARL/LNL/… GuC/HuC/DMC — not Arc blobs
    pci/                         # IGD 00:02.0 DID tables
    display/                     # package DE, GOP handoff, UMA FB
    submit/
    isa/                         # AIR → Xe-LPG / Xe2 (not Gen9 EU)
```

Bundle IDs: project IDs, **not** `com.apple.*`, not `AppleIntel*`. Freeze in Phase 0.

---

## 8. Do-nots

1. **Do not rewrite `host/`** to “make Intel easier.” Same contracts as Nvidia.
2. **Do not spoof `AppleIntelKBL*` / `ICL*` / UHD 630 / Iris Plus personalities onto Xe-LPG, Xe2, or Arc.** New IHV backend.
3. **Do not patch Apple kexts** (WhateverGreen / OCLP class). Trace only (`03` §4).
4. **Do not claim `i915` / `xe` / Mesa ANV will load on XNU.** New IOKit driver (`02` §7).
5. **Do not write SIP, OpenCore, AuxKC, 1TR, `ig-platform-id`, or unsigned-kext steps.** Loading is assumed solved. Failed attach → IORegistry → stop.
6. **Do not confuse Intel iGPU with Apple Silicon AGX / DCP.**
7. **Do not treat DG1 / Xe-LP “Iris Xe MAX” or datacenter Max as the first Arc board.**
8. **Do not mix Alchemist and Battlemage firmware or iGPU and Arc blobs.**
9. **Do not invent IOGPU selectors, GuC opcodes, or AIR opcodes.** UNKNOWN + cite.
10. **Do not replace** `Metal.framework`, `IOGPU.framework`, WindowServer, or the AIR front-end.
11. **Do not implement a DriverKit graphics family** Apple has not published.
12. **Do not lie in `supportsFamily`.**
13. If asked for an exploit, kext patch, or SIP bypass: refuse. Architecture only.

---

## 9. Open questions

Resolve from public headers, KDK (pinned Tahoe), Intel PRM/linux-firmware, and traces. Do not guess.

1. Host unknowns 1–12 (`CURSOR-IHV` §14) — IOGPU vs `IOAccelContext2` on Tahoe Intel, `CompilerPluginInterface`, AIR triples, dumb FB, KPI list.
2. First Arc board: Alchemist vs Battlemage? GuC **exact** version pairing for that DID.
3. First iGPU board: MTL Xe-LPG vs LNL Xe2? (Linux KMD default differs — i915 vs Xe — but we are not Linux.)
4. macOS-redistributable GuC/HuC/GSC/DMC license.
5. DG2/MTL GSC/MEI: is HuC required for **compute**, or media-only? (Public Xe docs: HuC is media/auth. Treat compute-without-HuC as the working hypothesis; **measure**.)
6. 2019 Mac Pro ReBAR / large BAR for Arc VRAM mapping.
7. iGPU stolen memory / DSM: does WindowServer care, or only Linux?
8. Cross-device IOSurface (Arc VRAM ↔ Apple/AMD UMA or iGPU) without CPU (`01` open q. 8).
9. Honest minimum `MTLGPUFamily` for Xe-HPG compute-only vs render vs Xe2.
10. Whether Intel still documents a public **Xe ISA** encoding sufficient to write an AIR backend, or only IGC/Mesa source as the spec.
11. Panther Lake Xe3 / Arc Pro B70: in-lab or stretch? Do not freeze a paper SKU.
12. Thunderbolt Arc eGPU: `IOPCITunnelCompatible` + AGDC participation — same host unknown as Nvidia (`CURSOR-IHV` §14 q. 10).

---

## 10. Sources

**On disk:** `CURSOR-IHV-DRIVER-SPEC.md` (host), `01-metal-userspace.md`, `02-kernel-boot-display.md`, `03-nvidia-prior-art.md`, `FINDINGS.md`.

**Apple host / policy:** [102363](https://support.apple.com/en-us/102363); [101988](https://support.apple.com/en-us/101988); [101641](https://support.apple.com/en-us/101641); [IOFramebuffer.h](https://github.com/apple-oss-distributions/IOGraphics/blob/main/IOGraphicsFamily/IOKit/graphics/IOFramebuffer.h); [Matching](https://developer.apple.com/library/archive/documentation/DeviceDrivers/Conceptual/IOKitFundamentals/Matching/Matching.html); [WWDC20-10615](https://developer.apple.com/videos/play/wwdc2020/10615/); [WWDC19-702](https://developer.apple.com/videos/play/wwdc2019/702/); [MTLCopyAllDevices](https://developer.apple.com/documentation/metal/mtlcopyalldevices()); [CAMetalLayer](https://developer.apple.com/documentation/quartzcore/cametallayer); [philipturner IORegistry](https://gist.github.com/philipturner/48c72e3fcce0ce9489071eb083a5086e); [01 §4.4 Intel table](01-metal-userspace.md).

**Apple last Intel iGPU (trace):** [MBP 13" 2020 specs](https://support.apple.com/en-euro/111339); [iMac 27" 2020](https://support.apple.com/en-us/111913); [Tahoe last Intel / Golden Gate](https://www.macrumors.com/2026/08/20/macos-golden-gate-marks-the-end-of-an-era/); [WhateverGreen kern_igfx.cpp](https://github.com/acidanthera/WhateverGreen/blob/008bfc3129e8e196beb44238437f325f2dd33e9d/WhateverGreen/kern_igfx.cpp); [Mojave SLE](https://gist.github.com/knightsc/6c188ad9c065d7c134de88564974952c).

**Arc / modern iGPU (public product):** [Intel Xe (Wikipedia)](https://en.wikipedia.org/wiki/Intel_Xe); [Intel Arc Pro B70](https://www.intel.com/content/www/us/en/products/sku/245797/intel-arc-pro-b70-graphics/specifications.html); [Intel Arc Pro B65](https://www.intel.com/content/www/us/en/products/sku/245796/intel-arc-pro-b65-graphics/specifications.html); [ACM PRM vol. 4 (A770 0x56A0)](https://kiwitree.net/~lina/intel-gfx-docs/prm/acm/intel-gfx-prm-osrc-acm-vol04-configurations.pdf); [Intel Arc graphics Windows driver SKU list](https://www.intel.com/content/www/us/en/download/785597/intel-arc-graphics-windows.html); [Gant A770 / Mac Pro 2019](http://blog.greggant.com/posts/2025/04/20/intel-arc-a770-mac-pro-2019-bad-idea.html); [Dortania: Intel GPUs unsupported](https://dortania.github.io/GPU-Buyers-Guide/modern-gpus/intel-gpu.html).

**Linux KMD / firmware (study, not a port):** [drm/xe](https://docs.kernel.org/gpu/xe/index.html); [Xe firmware](https://docs.kernel.org/gpu/xe/xe_firmware.html); [Xe RFC / i915 vs Xe](https://docs.kernel.org/next/gpu/rfc/xe.html); [Phoronix MTL i915 vs Xe](https://www.phoronix.com/review/intel-mtl-i915-xe-linux); [Phoronix Battlemage display](https://www.phoronix.com/news/Intel-Linux-Display-Battlemage); [compute-runtime #811](https://github.com/intel/compute-runtime/issues/811); [intel-xe firmware PR](https://lists.freedesktop.org/archives/intel-xe/2024-August/044362.html); [CDCLK](https://dri.freedesktop.org/docs/drm/gpu/intel-display/cdclk.html); [IGC FOSDEM](https://archive.fosdem.org/2024/events/attachments/fosdem-2024-2501-challenges-of-supporting-multiple-versions-of-llvm-in-intel-graphics-compiler/slides/22869/fosdem2024-llvm-in-igc_KVhVGa2.pdf).

**Blob-enablement (do not copy):** [WhateverGreen](https://github.com/acidanthera/WhateverGreen/blob/master/README.md); [WhateverRed](https://github.com/ainexur/WhateverRed).

---

*Architecture and implementation plan only. Loading kexts is out of scope. If a claim is not cited, treat it as UNKNOWN and look it up before coding.*
