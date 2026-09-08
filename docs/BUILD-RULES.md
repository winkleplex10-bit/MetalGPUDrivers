# Build rules — macOS GPU IHV project

**Audience:** every agent and human working this tree.  
**Status:** binding project policy as of 8 Sep 2026. Vendor CURSOR-START docs inherit these rules; where they conflict, **this file wins**.

---

## 1. Target OS

| Field | Rule |
|---|---|
| **Ship / product pin** | **macOS 26** (Tahoe) |
| **SDK / Darwin kext builds** | Target the **macOS 26** SDK (`xcrun --sdk macosx` on a Tahoe toolchain). Do not treat Sequoia as the ship pin. |
| **Bring-up evidence** | Older dumps (e.g. Sequoia 15.7.8) remain valid **lab history**. Re-validate attach, FB, and Metal acceptance on macOS 26 before claiming a phase green. |
| **Intel cutoff** | macOS 26 is the last major Intel macOS. Intel-host IHV work still targets 26; Apple Silicon work also targets 26+. |

---

## 2. WhateverGreen: absent

| Field | Rule |
|---|---|
| **Product config** | **WhateverGreen is absent.** The supported product shape does **not** load `WhateverGreen.kext`, Lilu graphics plugins for AGDP/connector spoof, or WEG boot-args (`-wegnoegpu`, `-wegnoigpu`, fake IDs, etc.). |
| **Why** | This project writes **new IHV backends**. Blob-enablement and personality spoofing are not the product path. A clean macOS 26 stack (our kexts + Apple frameworks) is the acceptance environment. |
| **Historical lab** | Sequoia traces that show WEG loaded are **baseline archaeology**, not a coexistence requirement. Do not reintroduce “WEG must stay loaded” as a hard product rule. |
| **Discrete GPUs** | May still be present. Primary display remains **connector-driven** (cable on our heads → our FB; cable on another GPU → that GPU’s FB). Do not require `-wegnoegpu` to “make room.” |
| **Do not** | Patch WEG, fork WEG, depend on WEG patch sites, or document install steps that install WEG for this project’s drivers. |

---

## 3. Platform branches

This repository has **two first-class platform tracks**. Neither blocks the other.

| Track | Host | Display intent | Tree |
|---|---|---|---|
| **x86 / Intel-Mac + Hackintosh** | Intel Mac or x86 Hackintosh | Full IHV: scanout + Metal on GPUs Apple never shipped | `ihv/amd-*`, `ihv/nvidia/`, `ihv/arc/`, `ihv/intel-igpu/` |
| **ARM / Apple Silicon** | Apple Silicon Mac (M-series) | **Accelerate display** for foreign or sidecar GPUs — not compute-only | `ihv/apple-silicon/` |

### 3.1 Apple Silicon track — challenges (document; do not block)

These are **known, first-class hard problems**. Record them, design around them, stop a phase on **UNKNOWN** with evidence — but **do not** cancel the Apple Silicon track or treat “impossible” as a project rule.

1. **DCP owns the built-in panel.** Apple’s Display Coprocessor drives the lid / internal scanout. A Thunderbolt or PCIe foreign GPU does not automatically become `CGDirectDisplayCopyCurrentMetalDevice` for the built-in display ([102363](https://support.apple.com/en-us/102363); research `02` §1.2).
2. **AGX remains the system GPU** unless and until an alternate path is proven. Replacing AGX/DCP is not a weekend port.
3. **TinyGPU is compute-only.** Existence of an Apple-signed PCIDriverKit dext for Ampere+ over TB proves enumeration + non-Metal compute — **not** WindowServer or `MTLDevice` for display ([TinyGPU](https://docs.tinygrad.org/tinygpu/)).
4. **No public Graphics DriverKit family.** FB + Metal product path is still **kext-shaped** (or another Apple-documented attach surface). PCIDriverKit = probe only unless Apple publishes graphics dext APIs.
5. **Matching is not Hackintosh PCI.** AS platforms use ADT / device trees, tunnelled PCIe, and different power/hotplug semantics than AM5 `IGPU@0`.
6. **Signed load / AuxKC / SIP** differ on AS. This repo still does **not** document bypass recipes; failed attach → collect IORegistry / ADT and stop.
7. **Cross-device present** (foreign VRAM ↔ AGX/DCP IOSurface without CPU) is largely **UNKNOWN**.
8. **Apple product policy** states aftermarket GPU drivers are “not compatible with macOS” — a **policy** statement, not a missing header. It does not delete the engineering track.

**Product stance for ARM:** ridiculously difficult, **in scope**, **not blocked**. Attack display acceleration in phases (enumerate → dumb/external FB if available → compute → AIR→ISA → MTLDevice → present). Prefer external displays on the foreign card’s own connectors first; built-in panel / DCP handoff is a later, explicitly labeled stretch.

Full plan: [CURSOR-START-APPLE-SILICON.md](CURSOR-START-APPLE-SILICON.md).

---

## 4. Shared engineering rules (all tracks)

1. Do **not** fork `Metal.framework`, `IOGPU.framework`, WindowServer, or the AIR front-end.
2. Do **not** spoof Apple kext personalities (X6000 onto Raphael/RDNA3, Kepler onto Ampere, `AppleIntel*` onto Xe, etc.).
3. Do **not** port Linux KMDs (`amdgpu.ko`, `nvidia.ko`, `i915.ko`, Nouveau) onto XNU.
4. Do **not** write SIP / OpenCore / AuxKC / 1TR / unsigned-kext **recipes** in product docs. Loading is assumed solved on the lab host; failed attach → dumps → stop.
5. If a selector, entitlement, RPC ID, or opcode is not in public docs, opened headers, or a cited trace: write **UNKNOWN** and stop that branch — do not invent.
6. Firmware blobs stay out of git unless a **macOS** redistribution grant is documented.
7. Honest `supportsFamily` / storage modes — under-claim until proven.
8. **Never say the Apple Silicon display track is impossible.** Say what is hard, which phase attacks it, and what UNKNOWN blocks the next signal.

---

## 5. Build commands (repo)

| Goal | Command |
|---|---|
| Portable tests (current) | `make test` → `ihv/amd-rdna2-igpu` ATOM/VFCT tests |
| Darwin kext (x86 IHV) | `make kext` on a Mac with the **macOS 26** SDK |
| Apple Silicon IHV | No kext target yet — scaffold under `ihv/apple-silicon/`; add `make` targets when code exists |

Darwin `Makefile`s should pin comments and acceptance notes to **macOS 26**, not Sequoia.

---

## 6. Doc precedence

1. `docs/BUILD-RULES.md` (this file) — OS pin, WEG policy, platform tracks  
2. `CURSOR-IHV-DRIVER-SPEC.md` (pending) — host contracts  
3. Per-vendor `docs/CURSOR-START-*.md` — IHV fill plans  
4. Per-slot `ihv/*/docs/BUILD.md` — local build notes (must not contradict §1–§2)

When updating an older CURSOR-START that still says “WEG required” or “Apple Silicon out of scope,” change it to match this file.
