# CURSOR-START-APPLE-SILICON — ARM display-acceleration track

**Audience:** Cursor coding agents filling `ihv/apple-silicon/`.  
**Product:** accelerate **display** (scanout + Metal present) for GPUs on **Apple Silicon** Macs — not a compute-sidecar-only project.  
**Binding project rules:** [BUILD-RULES.md](BUILD-RULES.md) (§1 macOS 26, §2 WhateverGreen absent, §3.1 AS challenges).  
**Host contracts:** same doors as x86 IHVs (`CURSOR-IHV-DRIVER-SPEC.md` pending) — do not invent a second Metal stack.  
**Date:** 8 Sep 2026.

**Hard rules.** Architecture and implementation plan only. No SIP / OpenCore / AuxKC / unsigned-kext recipes. No WhateverGreen. If a selector, entitlement, ADT key, DCP message, or IOGPU method is not in public docs, opened headers, or a cited dump: write **UNKNOWN** and stop that branch. **Do not cancel this track** because the problems below are hard.

---

## 1. Goal

Build an unofficial path so a foreign or sidecar GPU on an **Apple Silicon** Mac can participate in **accelerated display**:

1. Device enumerates and firmware is alive.  
2. A display surface exists that WindowServer (or an accepted present path) can use — prefer the **card’s own HDMI/DP** first.  
3. `MTLCopyAllDevices()` can return **our** `MTLDevice` (AGX may still also appear).  
4. A stock `.metallib` compute app runs on **our** ISA.  
5. A Metal render app presents via `CAMetalLayer`; for displays we drive, `CGDirectDisplayCopyCurrentMetalDevice` returns **us**.

**Done** is that ladder. TinyGPU-style compute-only, a private harness triangle that never reaches WindowServer, or “AGX forever owns every pixel” as a permanent product rule are **not-done**.

**Stance:** ridiculously difficult. Challenges in §3 are **real**. They are **documented so agents attack them**, not so the project stops.

---

## 2. Host vs create (same split as x86)

| Layer | HOST (Apple — do not rewrite) | This track creates |
|---|---|---|
| App GPU API | `Metal.framework` | Nothing. Apps keep linking Apple Metal. |
| Built-in panel today | **DCP** + AGX | Later stretch: coexistence or handoff — **UNKNOWN** path; do not claim lid takeover until traced |
| External heads on foreign GPU | — | Modeset + `IOFramebuffer` (or documented AS equivalent) on **card ports** |
| Accelerator | `IOGPU` / IOAccel SPI as measured on macOS 26 | Vendor submit path speaking those contracts |
| Plugin | `MetalPluginName` discovery | Our `*MTLDriver.bundle` |
| Shader front-end | AIR | AIR → vendor ISA (SASS / RDNA / Xe — by sub-backend) |
| First-party | AGX + DCP | **Study / coexist. Do not patch or spoof AGX.** |

x86 `host/` shells are reusable where the ABI matches. AS-specific match, power, and display glue live under `ihv/apple-silicon/` so x86 trees stay clean.

---

## 3. Challenges (first-class — attack, do not block)

| # | Challenge | Why it is hard | Attack posture |
|---|---|---|---|
| 1 | **DCP owns the lid** | Internal scanout is not a classic `IOFramebuffer` PCIe story | Prefer **external** connectors on the foreign GPU for Phase 2. Lid/DCP handoff = explicit later phase; UNKNOWN until traces exist |
| 2 | **AGX is the system Metal device** | Apps and WindowServer default to AGX | Ship an honest second `MTLDevice`; do not steal AGX’s registry identity |
| 3 | **TinyGPU ≠ display** | Proves TB enumerate + compute dext, not FB/Metal present | Use as Phase 1 existence proof only; product remains display+Metal |
| 4 | **No Graphics DriverKit family** | Apple has not published a graphics dext API ([WWDC19-702](https://developer.apple.com/videos/play/wwdc2019/702/)) | Kext / documented attach surfaces; PCIDriverKit = probe only |
| 5 | **ADT / tunnelled PCIe** | Not Hackintosh `IGPU@0` matching | Measure ADT + IORegistry on the frozen M-series + enclosure; freeze match keys in Phase 0 |
| 6 | **Hotplug / `0xFFFFFFFF`** | TB surprise-remove | MSI, yank-safe submit; same lesson as x86 TB notes |
| 7 | **Cross-device IOSurface** | Foreign VRAM ↔ AGX/DCP without CPU | CPU map fallback documented as temporary; zero-copy = UNKNOWN |
| 8 | **Policy vs engineering** | [102363](https://support.apple.com/en-us/102363) “not compatible” | Policy does not remove headers or cancel the track |
| 9 | **Signed load on AS** | AuxKC / pairing differs from Intel Hackintosh | Out of doc scope for recipes; failed attach → dumps → stop |
| 10 | **Compiler** | AIR→vendor ISA still new work | Same Asahi rule: compiler before API (Phase 4 before MTLDevice) |

**Rule:** saying “DCP makes display acceleration impossible” is **forbidden** as a project conclusion. Saying “Phase 2 targets card HDMI/DP first; lid is UNKNOWN” is correct.

---

## 4. Hardware scope

**In (Phase 0 freeze one board):**

- One Apple Silicon Mac (M-series generation frozen in `docs/boards.md`).  
- One attach path: Thunderbolt enclosure **or** other supported tunnel — freeze which.  
- One GPU backend initially (likely Nvidia Ampere+ GSP or AMD RDNA3 — reuse `ihv/nvidia/` or `ihv/amd-rdna3/` ISA/submit **knowledge**, AS-specific match/display under this tree).

**Out of first freeze:**

- Replacing AGX for the lid on day one.  
- Datacenter-only GPUs with no display engine.  
- Depending on WhateverGreen (absent per BUILD-RULES).

**2023 Mac Pro** is not a discrete GPU host for Apple’s product story ([101988](https://support.apple.com/en-us/101988)); do not assume internal PCIe GPU slots behave like 2019 Intel Mac Pro.

---

## 5. Phased plan

Do not skip phases. Stop-and-report on UNKNOWN. Challenges in §3 do **not** skip phases either.

| Phase | Goal | Accept |
|---|---|---|
| **AS0** | Freeze Mac model, OS build (**26**), enclosure, GPU DID, ADT/IORegistry baseline; WEG absent | `boards.md` complete; UNKNOWN list filed |
| **AS1** | Enumerate + firmware alive (GSP/PSP/GuC as applicable) | Heartbeat / no-op; still invisible to Metal |
| **AS2** | Dumb FB / modeset on **card’s own** HDMI/DP if present | External desktop or console path; lid may remain AGX/DCP |
| **AS3** | Non-Metal compute submit | Buffer round-trip |
| **AS4** | AIR → ISA offline | Trivial kernel correct on submit path |
| **AS5** | `*MTLDriver.bundle` | Our device in `MTLCopyAllDevices` |
| **AS6** | Present on displays **we** drive | `CAMetalLayer` + present; honest which displays |
| **AS7** | Built-in panel / DCP coexistence or handoff | Stretch — only after AS6 on external; UNKNOWN-heavy |

---

## 6. Repo layout

```
ihv/apple-silicon/
  README.md
  docs/
    boards.md          # Phase AS0 freeze
    challenges.md      # Living log of §3 attacks / evidence
    unknowns.md
  match/               # ADT / PCIe tunnel match — not x86 DID tables alone
  display/             # External heads first; DCP notes later
  submit/              # Glue to vendor submit (nvidia/amd/…) or local
  metal/               # *MTLDriver.bundle for AS attach
  firmware/            # License notes; no blobs unless granted
```

Vendor ISA trees stay in `ihv/nvidia/`, `ihv/amd-*`, etc. This folder owns **AS platform integration** and display-acceleration intent.

---

## 7. Do-nots

1. **Do not** declare the ARM display track out of scope or “impossible.”  
2. **Do not** require WhateverGreen or Lilu graphics patches.  
3. **Do not** patch AGX / DCP binaries or spoof AGX personality onto a foreign DID.  
4. **Do not** call TinyGPU “display done.”  
5. **Do not** block AS0–AS6 on solving lid/DCP first — external heads first.  
6. **Do not** invent DCP protocols or IOGPU selectors. UNKNOWN + cite.  
7. **Do not** write SIP/1TR/unsigned load recipes.  
8. **Do not** rewrite x86 IHV trees to “make AS easier” — share host contracts; keep AS glue here.

---

## 8. Open questions

1. macOS 26 AS: exact IORegistry / ADT nodes for TB GPU + displays.  
2. Whether an external-only `IOFramebuffer` on a foreign GPU is accepted by WindowServer beside DCP.  
3. IOGPU vs other user clients for a second `MTLDevice` on AS.  
4. `CompilerPluginInterface` on AS vs in-bundle compile.  
5. Cross-device IOSurface AGX ↔ foreign VRAM.  
6. Which vendor GPU is the first AS0 board (Ampere TB vs RDNA3 TB).  
7. Firmware redistrib on macOS for that vendor.

---

## 9. Sources

- [BUILD-RULES.md](BUILD-RULES.md)  
- [CURSOR-START-NVIDIA.md](CURSOR-START-NVIDIA.md) · [CURSOR-START-AMD.md](CURSOR-START-AMD.md)  
- [102363](https://support.apple.com/en-us/102363) · [101988](https://support.apple.com/en-us/101988)  
- [TinyGPU](https://docs.tinygrad.org/tinygpu/)  
- [Asahi AGX](https://asahilinux.org/docs/hw/soc/agx/) (hardware notes; wrong OS — do not port DRM)  
- [WWDC19-702](https://developer.apple.com/videos/play/wwdc2019/702/) · [WWDC20-10615](https://developer.apple.com/videos/play/wwdc2020/10615/)

---

*ARM display-acceleration track. Hard ≠ blocked. If a claim is not cited, treat it as UNKNOWN.*
