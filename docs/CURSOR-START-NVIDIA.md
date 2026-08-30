# CURSOR-START-NVIDIA — unofficial Ampere+ display + Metal IHV backend

**Audience:** Cursor coding agents filling `ihv/nvidia/` in the unofficial IHV GPU stack.  
**Host contract (binding; do not rewrite):** `CURSOR-IHV-DRIVER-SPEC.md`. This file is the **Nvidia IHV slot**. Matching, `IOFramebuffer` shell, IOGPU-speaking accelerator shell, `*MTLDriver.bundle` skeleton, AIR ingest, present glue, and power-helper shape live in `host/` and stay vendor-agnostic.  
**Research corpus (cite; do not invent APIs):** `01-metal-userspace.md`, `02-kernel-boot-display.md`, `03-nvidia-prior-art.md`, `FINDINGS.md`.  
**Date:** 29 Aug 2026. Host spec: 29 Aug 2026. Corpus: 27 Aug 2026.  
**Hard rule:** architecture and implementation plan only. No SIP/OpenCore/unsigned-kext recipes (loading is assumed solved on the Intel x86 dev/Hackintosh host). No exploits. If a selector, entitlement, GSP RPC ID, class header name, or AIR opcode is not in the corpus, a public Apple/Nvidia header you have opened, or a URL cited here, write **UNKNOWN** and stop that branch.

---

## 1. Goal

Build a **usable unofficial display + Metal driver** for modern Nvidia so an Ampere-or-newer GSP-era card on an Intel x86 Mac / Hackintosh enumerates as an IHV GPU: WindowServer attaches to our `IOFramebuffer`, `MTLCopyAllDevices()` returns our `MTLDevice`, a stock `.metallib` compute app runs on **SASS**, and a Metal render app presents via `CAMetalLayer` onto the display we drive (`CURSOR-IHV-DRIVER-SPEC` §1, §3). This is **ridiculously difficult**, not a weekend port, and **not impossible**. Kepler/Pascal Web Drivers and CUDA 10.2 are **fossils**, not this tree (`03` §3). TinyGPU is a **compute-only dext probe**, not the product (`01` §1; [tinygrad TinyGPU](https://docs.tinygrad.org/tinygpu/)). Ada Lovelace and consumer Blackwell occupy the **same GSP + open-rm slot** once Ampere bring-up is real. Apple Silicon stays a later **compute sidecar**: AGX remains the only system `MTLDevice`; a Thunderbolt Nvidia card does not replace DCP (`02` §1.2; [102363](https://support.apple.com/en-us/102363)).

---

## 2. What Apple already has vs what this Nvidia tree must create

Apple owns the host. We fill the Nvidia box behind the doors in `CURSOR-IHV-DRIVER-SPEC` §5. Do **not** fork Apple frameworks.

| Layer | Apple already ships (HOST — do not rewrite) | This `ihv/nvidia/` tree creates |
|---|---|---|
| App GPU API | `Metal.framework` (`MTLDevice`, queues, `MTLCopyAllDevices`, `supportsFamily`) | Nothing. Apps keep linking Apple Metal. |
| Compositor / drawables | WindowServer / SkyLight; `CAMetalLayer`; `IOSurface` | GPU-map IOSurface BOs on **this** card; IOSurface-backed present |
| Display family | `IOGraphicsFamily` / `IOFramebuffer` class ([`IOFramebuffer.h`](https://github.com/apple-oss-distributions/IOGraphics/blob/main/IOGraphicsFamily/IOKit/graphics/IOFramebuffer.h)) | Subclass in `host/iofb/`; **program Nvidia display engine** (heads / SOR / HDMI-DP) in `ihv/nvidia/display/` |
| Accelerator family | `IOAcceleratorFamily2` / `IOGPUFamily` (private SPI) | Speak those user clients from `host/ioaccel/`; **GSP RPC + compute/3D classes** in `ihv/nvidia/submit/` |
| Plugin discovery | Metal `dlopen`s `MetalPluginName` / `MetalPluginClassName` ([philipturner gist](https://gist.github.com/philipturner/48c72e3fcce0ce9489071eb083a5086e)) | Our `*MTLDriver.bundle`; registry keys on **our** accelerator. Historical `GeForceMTLDriver.bundle` is **slot evidence only** (`01` §4.5) |
| Shader front-end | `metal` / `metallib` → AIR in `.metallib` ([WWDC20 10615](https://developer.apple.com/videos/play/wwdc2020/10615/)) | **AIR → SASS** in `ihv/nvidia/isa/`. No public AIR→PTX/SASS (`01` §5.3) |
| PCI | `IOPCIFamily`; `IOPCITunnelCompatible` if TB | VID `10de` + one Ampere+ DID allow-list (`ihv/nvidia/pci/`) |
| First-party GPU drivers | AGX, Intel iGPU, AMD `AMDRadeonX6000*` | **Do not ship, patch, or spoof.** ABI study of AMD plugin on the same OS only |
| Compute dext | PCIDriverKit (TinyGPU’s shape) | Optional Phase 1 **BAR/GSP probe only**; product path is **kexts** (no Graphics DriverKit family) (`02` §3.3) |

**One-line split:** `host/` speaks Apple. `ihv/nvidia/` boots GSP, programs display, submits Ampere+ classes, and lowers AIR to SASS.

---

## 3. Hardware scope

**In (Phase 1 board):** one **Ampere** discrete card (GA10x, GeForce RTX 30 / matching RTX A). GSP host-bootstrap is required. Same floor TinyGPU chose: Pascal-class Option-ROM POST is not how modern cards init (`02` §5.4; `03` §6.3; [TinyGPU](https://docs.tinygrad.org/tinygpu/)). PCI vendor is `0x10de`. Freeze **one** DID from `system_profiler` / IORegistry into `docs/boards.md`. Example Ampere DIDs exist in Nvidia’s open-rm Compatible GPUs table (e.g. RTX 3090 `2204`, RTX 3080 `2206`, RTX 3070 `2484`) — copy from that table, do not invent ([open-gpu-kernel-modules README](https://github.com/NVIDIA/open-gpu-kernel-modules)).

**Same slot, later boards (do not start here):** **Ada Lovelace** (AD10x, RTX 40; open-gpu-doc `ADA_A` / `clc997.h`) and **consumer Blackwell** (GB20x, RTX 50; open-gpu-doc 3D `clcd97`/`clce97`, compute `clcdc0`/`clcec0`). Public facts that make this the same IHV backend, not a new host:

- GSP was **first introduced in Turing**. Open kernel modules **require** GSP and **cannot** support pre-Turing ([Nvidia kernel_open README](https://download.nvidia.com/XFree86/Linux-x86_64/575.51.02/README/kernel_open.html); [GSP firmware chapter](https://download.nvidia.com/XFree86/Linux-x86_64/575.57.08/README/gsp.html)).
- Ampere / Ada / Hopper: Nvidia **recommends** open-rm. **Blackwell and later: open-rm only**; proprietary `nvidia.ko` flavor is unsupported ([Nvidia R560 blog](https://developer.nvidia.com/blog/nvidia-transitions-fully-towards-open-source-gpu-kernel-modules/)).
- NVK advertises Kepler→Ada plus consumer Blackwell as Vulkan 1.4 ([Mesa NVK](https://docs.mesa3d.org/drivers/nvk.html); [Collabora Mesa 25.2](https://www.collabora.com/news-and-blog/news-and-events/mesa-25.2-brings-new-hardware-support-for-nouveau-users.html)).

Generation diffs (QMD layout, surface kind, texture encoding) live in `ihv/nvidia/` tables, not in `host/`.

**Out:**

| Chip | Why out |
|---|---|
| Kepler | Apple Metal 1 blob removed in Monterey (`03` §3.5; [OCLP #498](https://github.com/dortania/OpenCore-Legacy-Patcher/issues/498)). Fossil restore, not this project. |
| Maxwell / Pascal | Web Driver Metal 1 on **High Sierra 10.13.6 only** ([Nvidia 387…](https://www.nvidia.com/en-us/drivers/details/156748/)). No GSP. Open-rm incompatible. Falcon/ACR + Option-ROM POST — TinyGPU explicitly skipped. |
| Volta | Pre-GSP for open-rm purposes (Nvidia: Maxwell/Pascal/Volta stay proprietary). |
| Turing (TU10x) | Has GSP, so hardware-capable, but **not** the Phase 1 board. TinyGPU / host floor is Ampere+. Do not pick TU10x to “make GSP easier.” |
| Hopper / datacenter (GH100, B200, …) | GSP-era compute. Display-engine + WindowServer story is **UNKNOWN**. Not a Phase 1 display card. |
| Mobile MX / laptop dGPU in a non-Mac chassis | Out unless it is the exact frozen board. |

**Host machine:** Intel x86 Mac or Hackintosh x86. Preferred: 2019 Mac Pro (MacPro7,1) PCIe or Intel TB3 eGPU. 2023 Mac Pro is **not** a graphics-card host ([101988](https://support.apple.com/en-us/101988)). macOS **26 Tahoe** pin (`CURSOR-IHV-DRIVER-SPEC` §4).

**Apple Silicon:** AGX + DCP always own the built-in panel. A TB Nvidia card is a **compute sidecar** later (TinyGPU-class), labeled as such. It cannot become `CGDirectDisplayCopyCurrentMetalDevice` for the lid.

**GSP is non-negotiable.** Open-rm maintainers: open modules **unconditionally require** GSP; `NVreg_EnableGpuFirmware=0` is a no-op there ([open-gpu-kernel-modules discussion #667](https://github.com/NVIDIA/open-gpu-kernel-modules/discussions/667); [#820](https://github.com/NVIDIA/open-gpu-kernel-modules/issues/820)). Do not plan a “bit-bang Ampere without firmware” path.

---

## 4. Public study sources (hardware knowledge; OS integration does not transfer)

Read these. Do **not** load them on XNU. Do **not** copy Linux UAPI, DRM, or Mesa winsys into `host/`.

| Source | What to steal | What not to steal |
|---|---|---|
| **open-rm** [`NVIDIA/open-gpu-kernel-modules`](https://github.com/NVIDIA/open-gpu-kernel-modules) (dual MIT/GPLv2 kernel modules; current tree cites driver **610.57.04**) | GSP boot, RPC queues, RM classes, `nvidia-modeset` / NVKMS display objects, DID table, `src/nvidia/src/kernel/gpu/gsp/` | `nvidia.ko` / `nvidia-drm.ko` / `nvidia-modeset.ko` as binaries. Linux `kernel-open/` interface layer. |
| **open-gpu-doc** [`nvidia/open-gpu-doc`](https://github.com/nvidia/open-gpu-doc) ([site](https://nvidia.github.io/open-gpu-doc/); [LICENSE.md MIT-style](https://nvidia.github.io/open-gpu-doc/LICENSE.md)) | 3D/compute class headers (`AMPERE_A` `clc697`, `AMPERE_B` `clc797`, `ADA_A` `clc997`, Blackwell `clcd97`/`clce97`; compute QMD headers). DCB / Devinit / Shader-Program-Header. Keep copyright notice if you ingest headers. | Treating class headers as a Metal SDK. Display-engine programming still needs modeset/GSP, not only `clxx97`. |
| **NVK + NAK** ([Mesa NVK](https://docs.mesa3d.org/drivers/nvk.html); [Collabora introducing NVK](https://www.collabora.com/news-and-blog/news-and-events/introducing-nvk.html); [NVK hardware docs](https://docs.mesa3d.org/drivers/nvk/external_hardware_docs.html)) | Command-buffer class usage; **NAK** = NIR → SASS (Maxwell+ control info, Volta+ reconvergence). Blackwell notes (QMD, surface kind, bindless) ([Airlie](https://airlied.blogspot.com/2025/07/nvk-blackwell-support.html)). | Gallium/NIR/Vulkan. NAK does **not** ingest AIR. Zink is OpenGL-on-Vulkan, not Metal. |
| **Nouveau GSP + Nova** ([nouveau wiki](https://nouveau.pages.freedesktop.org/wiki/); [Nova](https://rust-for-linux.com/nova-gpu-driver); [GSP RPC design](https://lists.freedesktop.org/archives/dri-devel/2025-October/531478.html)) | Thesis: **boot GSP + RPC**, do not reverse-engineer the 3D engine. Nova-core = PCI + GSP + queues; Nova-DRM = Linux uAPI. Maxwell2+ signed firmware blocked reclocking without GSP. | DRM uAPI, Rust-for-Linux scaffolding, VFIO/vGPU. |
| **TinyGPU** ([docs](https://docs.tinygrad.org/tinygpu/); [heise](https://www.heise.de/en/news/First-eGPU-drivers-for-Apple-Silicon-Macs-but-only-for-AI-11246984.html)) | Existence proof: Ampere+ over TB **enumerates** and can run **non-Metal** compute with an Apple-signed PCIDriverKit dext. NVCC/Docker compiler path. | Product architecture. No `MTLDevice`, no WindowServer, no scanout. Do not paste tinygrad source. |
| **Asahi playbook** (`03` §7.1) | Trace → structure decode → submit path → compiler before API. | Mesa/DRM winsys; driving *Apple’s* GPU from Linux. |
| **Historical Mac Nvidia** (`01` §4.5; `03` §3) | Plugin *shape*: `NVDAResman` + `GeForceMTLDriver.bundle` + `GeForceAIRPlugin.bundle`. | Any blob reuse. Web Drivers ended at 10.13.6. CUDA 10.2 last Mac CUDA ([CUDA notes](https://docs.nvidia.com/cuda/archive/10.2/cuda-toolkit-release-notes/index.html)). |

**Firmware blobs:** linux-firmware GSP images (`gsp_*.bin`, named by arch, e.g. `gsp_tu10x.bin`) are a Linux redistribution ([GSP chapter](https://download.nvidia.com/XFree86/Linux-x86_64/575.57.08/README/gsp.html)). A **macOS redistribution grant is UNKNOWN** (`03` §10.3). Do not commit blobs unless license says so. Do not flash unsigned firmware.

---

## 5. Problem areas specific to Nvidia

These are first-class. Host-wide challenges (IOGPU ABI, WindowServer FB, no Graphics DriverKit) are in `CURSOR-IHV-DRIVER-SPEC` §10 — attack those there. Below is **only** what is Nvidia-shaped.

| Problem | Why it is hard | Attack |
|---|---|---|
| **AIR → SASS is new work** | Apps ship AIR inside `.metallib`. Apple AIRNT plugins know `applegpu_g13*`…`g18p` only ([zboralski gist](https://gist.github.com/zboralski/524d292e8c1fa5cb64500a85874a333b)). NAK compiles **NIR**, not AIR. Historical `GeForceAIRPlugin.bundle` is Kepler/Maxwell Metal 1, not a retargetable backend (`01` §5). PTX/`nvdisasm` docs hint at ISA; they are not a Metal ingest. | Phase 4 **offline** before `MTLDevice`: parse a trivial `.air`/`.metallib` you compiled with `xcrun metal`; emit Ampere SASS; run via Phase 3 submit. If `CompilerPluginInterface` cannot register, compile inside `*MTLDriver.bundle` at PSO time (host open q. 3). |
| **GSP is the HAL** | Turing+ RM lives in RISC-V firmware. Host talks shared-memory RPC, not 3D MMIO ([Nova](https://rust-for-linux.com/nova-gpu-driver); [GSP chapter](https://download.nvidia.com/XFree86/Linux-x86_64/575.57.08/README/gsp.html)). Bad pointers brick the session (Asahi AGX lesson). TinyGPU’s GSP **graphics vs compute** feature set on macOS is UNKNOWN (`03` open q. 4). | BAR map → load licensed GSP → heartbeat → no-op RPC → compute class. Log RPC class IDs as **observed**, never guessed. Stop if license blocks blobs. |
| **No living Mac Metal plugin from Nvidia** | Last official Metal on Nvidia: Apple Kepler Metal 1 (gone Monterey) and Web Driver Metal 1 for Maxwell/Pascal on High Sierra only (`03` §3.5). There is **no** Ampere/Ada/Blackwell `*MTLDriver.bundle`. WhateverGreen/OCLP only **enabled old blobs** (`03` §4). | Implement a **new** plugin. ABI-study Apple’s **AMD** `*MTLDriver.bundle` on Tahoe Intel (same OS). Do not link Apple AMD code. Do not revive `com.nvidia.web.*`. |
| **Display engine ≠ NVDEC ≠ shader cores** | NVDEC “runs completely independent of compute/graphics engine” ([NVDEC guide](https://docs.nvidia.com/video-technologies/video-codec-sdk/13.0/nvdec-video-decoder-api-prog-guide/index.html)). Scanout is NVDisplay / EVO / NVKMS (`nvidia-modeset`, heads, windows, connectors, SOR) ([open-rm display](https://github.com/NVIDIA/open-gpu-kernel-modules)). `IOFramebuffer.h` already splits FB from `IOAccelerator`. Video decode is **not** Phase 2. | Phase 2: dumb linear FB / modeset on **display** IP. Phase 3: compute on **GSP + GR**. Do not block FB on NVDEC or AIR. |
| **Signed firmware / clocks** | Maxwell2+ signed firmware blocked Nouveau reclocking ([nouveau wiki](https://nouveau.pages.freedesktop.org/wiki/)). Ampere without GSP is not an open-rm path. | Only GSP-brokered clocks. No “unsigned Falcon” plan. |
| **Discrete VRAM vs UMA** | Metal `Managed`/`Private` exist because of this (`01` §3.1). Cross-device IOSurface without CPU is UNKNOWN (host open q. 9). | Phase 3 CPU map round-trip first. Present: GPU-map IOSurface into this card’s VA. |
| **Thunderbolt `0xFFFFFFFF`** | Surprise-remove and Pause (`02` §6.1). Blackwell eGPU/OCuLink GSP RPC timeouts are a **Linux** cautionary tale ([open-rm #1247](https://github.com/NVIDIA/open-gpu-kernel-modules/issues/1247)) — not a Mac recipe, but GSP+tunnel is fragile. | MSI not INTx. Treat `0xFFFFFFFF` as yank. Prefer internal PCIe for Phase 1. |
| **Blackwell deltas** | NVK: separate depth/stencil, generic memory kind, texture-handle encoding ([Airlie](https://airlied.blogspot.com/2025/07/nvk-blackwell-support.html)). Open-rm **mandatory** on GB20x. | Keep Ampere as first ISA/QMD. Gate Blackwell behind generation tables after P4 green on GA10x. |

---

## 6. Phased starting plan (Nvidia fill of host §11)

Do not skip phases. Do not start a Metal plugin on day one. Compiler before API (Asahi). Each phase **stop-and-report** if blocked by UNKNOWN. Host P0/P7 still apply; this section is the Nvidia specialization of P1–P6.

### Phase 0 — freeze (host P0, Nvidia notes)

Pick Intel x86 host + **one** Ampere board. Pin Tahoe build. Snapshot working **AMD** IORegistry (`MetalPluginName`, accelerator class) as ABI oracle. File UNKNOWN list (GSP redistrib, IOGPU selectors, entitlements). Ampere+ only.

**Accept:** host P0.1–P0.5. Extra: `ihv/nvidia/pci/` has exactly one DID; Kepler/Pascal marked not selected.

### Phase 1 — enumerate + firmware alive

**Goal:** personality attaches; BARs mapped; GSP heartbeats; one doorbell/RPC no-op completes. Throwaway PCIDriverKit probe allowed; product path remains the kext.

| ID | Criterion |
|---|---|
| N1.1 | `IOPCIDevice` nub; link trained (`02` §5.1) |
| N1.2 | Our `IOClass` wins `start` on VID `10de` + frozen DID |
| N1.3 | Config-space identity readable; documented Nvidia identity register if a **public** header names one, else skip (do not invent `PMC_BOOT_0` from community blogs into shipping code unless you open the header) |
| N1.4 | GSP boot reaches a logged ready state **or** UNKNOWN + vendor-license block. Do not bit-bang 3D MMIO |
| N1.5 | One RPC/doorbell no-op or identity fill completes without panic |
| N1.6 | `MTLCopyAllDevices` still does **not** list us |

### Phase 2 — dumb framebuffer / WindowServer

**Goal:** `IOFramebuffer` subclass; modeset; desktop on the **card’s own HDMI/DP** (2019 Mac Pro has no iGPU; Mac TB ports need MPX — [101641](https://support.apple.com/en-us/101641)).

| ID | Criterion |
|---|---|
| N2.1 | FB service registered; console/panic can paint |
| N2.2 | WindowServer opens the user client; login window or desktop visible |
| N2.3 | Cursor + VBL stubbed; modes published |
| N2.4 | Unaccelerated OK; no `MTLDevice`; NVDEC unused |

If WindowServer refuses a dumb FB: capture the user-client error, implement the **minimum** extra methods the trace shows — do not skip to Metal to “fix” display (host P2).

### Phase 3 — non-Metal compute submit

**Goal:** BO alloc; GSP RPC / Ampere compute class; known pattern in a mapped buffer. Still invisible to Metal.

| ID | Criterion |
|---|---|
| N3.1 | Alloc/free device-visible memory; CPU map round-trip |
| N3.2 | Submit + wait; buffer contains expected bytes |
| N3.3 | Reset does not panic; TB yank (if TB) does not spin on `0xFFFFFFFF` |
| N3.4 | Still invisible to Metal |

Use open-gpu-doc compute class for **this** chip (Ampere: public `clc6c0`/`clc7c0`-class names are in open-gpu-doc / open-rm class lists — **open the header**, do not copy IDs from memory). QMD version is a generation table, not a guess.

### Phase 4 — AIR compile one shader (not yet MTLDevice)

**Goal:** trivial AIR compute shader → SASS → Phase 3 submit. Metal.framework **not** in the loop.

| ID | Criterion |
|---|---|
| N4.1 | Parse `.air` / `.metallib` slice; reject unknown triple with a clear error |
| N4.2 | Emit SASS for `kernel void add(device uint* p [[buffer(0)]]) { p[0] += 1; }` (or equivalent you own) |
| N4.3 | That binary runs via N3 submit; output buffer correct |
| N4.4 | No `MTLCopyAllDevices` change |

### Phase 5 — MTLDevice

**Goal:** our bundle loaded via `MetalPluginName`. Tiny Metal compute app with a normal `.metallib` runs on the card.

| ID | Criterion |
|---|---|
| N5.1 | Accelerator registry: `MetalPluginName` / `MetalPluginClassName` → **our** bundle (not `GeForceMTLDriver`) |
| N5.2 | `MTLCopyAllDevices()` includes a device whose `registryID` matches our accelerator |
| N5.3 | `MTLCreateSystemDefaultDevice` may still be Apple/AMD — tests pick **ours** by registryID / name |
| N5.4 | `makeLibrary` → compute PSO → `commit` → result in `MTLBuffer` |
| N5.5 | `supportsFamily` honest (under-claim; do not claim `MTLGPUFamilyMac2` / Metal 3/4 until true) (`01` §3.1) |

### Phase 6 — present

**Goal:** Metal render + present on the Phase 2 FB.

| ID | Criterion |
|---|---|
| N6.1 | `CAMetalLayer` on our `MTLDevice`; `nextDrawable` is IOSurface-backed |
| N6.2 | `presentDrawable` + WindowServer composite; on-screen triangle / clear color |
| N6.3 | `CGDirectDisplayCopyCurrentMetalDevice` for our display returns our device |
| N6.4 | No CPU blit labeled as present unless documented as a temporary fallback |

Ada/Blackwell: after N4–N6 green on Ampere, add DID + generation tables only. That is host Phase 7 **within** `ihv/nvidia/`, not a host rewrite. Arc/RDNA3 remain other IHV directories.

---

## 7. Suggested repo paths under `ihv/nvidia/`

Align with `CURSOR-IHV-DRIVER-SPEC` §12. Do not put SASS or GSP inside `host/`.

```
ihv/nvidia/
  README.md                 # this slot; points at CURSOR-START-NVIDIA.md
  pci/
    personalities.md        # VID 10de, frozen DID, IOPCITunnelCompatible notes
    did-allowlist.txt       # Phase 0 one-line; later Ada/Blackwell append-only
  firmware/
    README.md               # how GSP is obtained; license; DO NOT git blobs unless granted
    gsp/                    # boot + RPC client (source); blob path is build-time
  display/
    heads/                  # CRTC / NVDisplay / EVO analogue — scanout, VBL, cursor
    or/                     # SOR / HDMI / DP link (card’s own ports)
    fb/                     # glue to host/iofb subclass (pitch, modes, sense)
  submit/
    rpc/                    # GSP RPC messages (logged IDs)
    fifo/                   # GPFIFO / channel / doorbell
    compute/                # Ampere compute class + QMD
    copy/                   # DMA copy class (later present)
  isa/
    air-ingest/             # thin wrap of host/air (triples, metallib slices)
    sass/                   # AIR → SASS (Ampere first; Ada/Blackwell tables later)
    samples/                # owned .metal / .air / expected buffer (Phase 4)
  power/
    clocks.md               # GSP-brokered only; no AGPM injector
  docs/
    abi-notes.md            # observed RPC, class headers opened, Tahoe traces
```

`host/` must compile against a stub IHV. Adding Arc must not edit these ISA files.

**Bundle IDs:** project IDs, not `com.apple.*`, not `com.nvidia.web.*`, not `GeForceWeb` (`01` §4.5; host §7). Freeze in Phase 0.

---

## 8. Do-nots

1. **Do not load `nvidia.ko`** (or `nvidia-drm` / `nvidia-modeset` / Nouveau / Nova) on XNU. Wrong kernel, wrong UAPI (`02` §7). Port hardware programming into IOKit.  
2. **Do not WhateverGreen-spoof** Kepler/Pascal/AMD IDs, AGDP board-ids, or Web Driver compat checks (`03` §4). That cannot create Ampere Metal.  
3. **Do not fork `IOGPU.framework`**, `Metal.framework`, WindowServer, or the AIR front-end. Speak user clients; ingest AIR; publish a FB.  
4. **Do not write SIP / OpenCore / AuxKC / 1TR / unsigned-kext steps.** If the kext does not attach, collect IORegistry and stop.  
5. **Do not revive Web Drivers, CUDA 10.2, `GeForce.kext`, or Kepler bundles** as product code.  
6. **Do not implement a DriverKit graphics family** Apple has not published. PCIDriverKit = Phase 1 probe only.  
7. **Do not patch Apple AMD/AGX/Intel kexts** or redistribute them.  
8. **Do not bit-bang the 3D engine** to skip GSP.  
9. **Do not treat NVDEC as display** or block scanout on video decode.  
10. **Do not claim `nvidia.ko` will load**, that AIR→SASS already exists, or that TinyGPU is a Metal device.  
11. **Do not start on Apple Silicon for display.** AGX stays the system GPU.  
12. **Do not invent IOGPU selectors, GSP RPC IDs, or `MetalPluginClassName` vtables.** UNKNOWN + trace.

---

## 9. Open questions

Resolve from public headers, the pinned Tahoe KDK, Nvidia docs, and traces. Do not guess. Host §14 still applies; these are Nvidia extras.

1. **macOS redistribution** of GSP / SEC2 / related signed firmware (linux-firmware ≠ a Mac grant). Blocks N1.4 if unanswered.  
2. GSP **graphics vs compute** feature set TinyGPU actually boots vs full HAL needed for modeset + 3D (`03` open q. 4).  
3. Whether Tahoe Intel discrete GPUs still use **IOGPU** user clients vs `IOAccelContext2` — measure X6000, then implement **that** table (host open q. 2).  
4. Whether `CompilerPluginInterface` can register a non-Apple AIR→SASS backend, or compilation must live inside `*MTLDriver.bundle` (host open q. 3).  
5. Exact Ampere **compute class + QMD version** for the frozen DID — open the matching open-gpu-doc header and cite the filename in `docs/abi-notes.md`.  
6. Display-engine class for GA10x (open-rm lists `NVC670_DISPLAY` / related NVDisplay classes on Ampere) — confirm against the header for **this** chip before programming heads.  
7. Honest minimum `MTLGPUFamily` for Ampere compute-only vs render (host open q. 11).  
8. Cross-device IOSurface without CPU between Nvidia VRAM and Apple/AMD (`01` open q. 8).  
9. AGDC / GPUWrangler required for **internal** PCIe FB attach on Tahoe, or only tunneled TB (`02` §2.3). Do not patch Apple checks.  
10. Blackwell-on-Mac: surface-kind / QMD v3 deltas vs Ampere — UNKNOWN until Ampere P4 is green; do not pre-implement GB20x ISA.  
11. Hopper / datacenter display: UNKNOWN; not Phase 1.  
12. `IOGraphicsFamily` KPI third-party status on this SDK ([Deprecated Kernel Extensions](https://developer.apple.com/support/kernel-extensions) — re-read; `02` open q. 8).

---

## 10. Source URLs

**On disk:** `CURSOR-IHV-DRIVER-SPEC.md`, `01-metal-userspace.md`, `02-kernel-boot-display.md`, `03-nvidia-prior-art.md`, `FINDINGS.md`.

**Apple policy / platform:** [102363](https://support.apple.com/en-us/102363) · [101988](https://support.apple.com/en-us/101988) · [101641](https://support.apple.com/en-us/101641) · [101644](https://support.apple.com/en-us/101644) · [Tahoe 26 notes](https://developer.apple.com/documentation/macos-release-notes/macos-26-release-notes) · [IOFramebuffer.h](https://github.com/apple-oss-distributions/IOGraphics/blob/main/IOGraphicsFamily/IOKit/graphics/IOFramebuffer.h) · [Matching](https://developer.apple.com/library/archive/documentation/DeviceDrivers/Conceptual/IOKitFundamentals/Matching/Matching.html) · [TB basics](https://developer.apple.com/library/archive/documentation/HardwareDrivers/Conceptual/ThunderboltDevGuide/Basics01/Basics01.html) · [PCIDriverKit](https://developer.apple.com/documentation/pcidriverkit/creating-custom-pcie-drivers-for-thunderbolt-devices) · [WWDC19-702](https://developer.apple.com/videos/play/wwdc2019/702/) · [WWDC20-10615](https://developer.apple.com/videos/play/wwdc2020/10615/) · [WWDC22-110373](https://developer.apple.com/videos/play/wwdc2022/110373/) · [MTLCopyAllDevices](https://developer.apple.com/documentation/metal/mtlcopyalldevices()) · [supportsFamily](https://developer.apple.com/documentation/metal/mtldevice/supportsfamily(_:)) · [CAMetalLayer](https://developer.apple.com/documentation/quartzcore/cametallayer)

**Nvidia open-rm / GSP / docs:** [open-gpu-kernel-modules](https://github.com/NVIDIA/open-gpu-kernel-modules) · [kernel_open (575.51.02)](https://download.nvidia.com/XFree86/Linux-x86_64/575.51.02/README/kernel_open.html) · [GSP firmware (575.57.08)](https://download.nvidia.com/XFree86/Linux-x86_64/575.57.08/README/gsp.html) · [R560 open-module transition](https://developer.nvidia.com/blog/nvidia-transitions-fully-towards-open-source-gpu-kernel-modules/) · [GSP required on open-rm (#667)](https://github.com/NVIDIA/open-gpu-kernel-modules/discussions/667) · [GSP cannot be disabled (#820)](https://github.com/NVIDIA/open-gpu-kernel-modules/issues/820) · [open-gpu-doc](https://github.com/nvidia/open-gpu-doc) · [open-gpu-doc site](https://nvidia.github.io/open-gpu-doc/) · [open-gpu-doc LICENSE](https://nvidia.github.io/open-gpu-doc/LICENSE.md) · [NVDEC independent of graphics](https://docs.nvidia.com/video-technologies/video-codec-sdk/13.0/nvdec-video-decoder-api-prog-guide/index.html)

**NVK / Nova / Nouveau:** [Mesa NVK](https://docs.mesa3d.org/drivers/nvk.html) · [NVK external hardware docs](https://docs.mesa3d.org/drivers/nvk/external_hardware_docs.html) · [Collabora introducing NVK](https://www.collabora.com/news-and-blog/news-and-events/introducing-nvk.html) · [Collabora Mesa 25.2 Blackwell](https://www.collabora.com/news-and-blog/news-and-events/mesa-25.2-brings-new-hardware-support-for-nouveau-users.html) · [Airlie NVK Blackwell](https://airlied.blogspot.com/2025/07/nvk-blackwell-support.html) · [Nova](https://rust-for-linux.com/nova-gpu-driver) · [nouveau wiki](https://nouveau.pages.freedesktop.org/wiki/) · [GSP RPC queues](https://lists.freedesktop.org/archives/dri-devel/2025-October/531478.html)

**TinyGPU / fossils:** [TinyGPU](https://docs.tinygrad.org/tinygpu/) · [heise](https://www.heise.de/en/news/First-eGPU-drivers-for-Apple-Silicon-Macs-but-only-for-AI-11246984.html) · [Web Driver 387…](https://www.nvidia.com/en-us/drivers/details/156748/) · [CUDA 10.2 last macOS](https://docs.nvidia.com/cuda/archive/10.2/cuda-toolkit-release-notes/index.html) · [OCLP #498](https://github.com/dortania/OpenCore-Legacy-Patcher/issues/498) · [WhateverGreen](https://github.com/acidanthera/WhateverGreen/blob/master/README.md)

**Plugin / compiler naming:** [philipturner IORegistry](https://gist.github.com/philipturner/48c72e3fcce0ce9489071eb083a5086e) · [khronokernel part 2](https://khronokernel.com/macos/2022/11/01/LEGACY-METAL-PART-2.html) · [AIRNT gist](https://gist.github.com/zboralski/524d292e8c1fa5cb64500a85874a333b) · [Asahi AGX](https://asahilinux.org/docs/hw/soc/agx/)

---

*Nvidia IHV slot only. Host plan is `CURSOR-IHV-DRIVER-SPEC.md`. Loading kexts is out of scope. If a claim is not cited, treat it as UNKNOWN.*
