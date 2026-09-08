# Building RaphaelIGPU.kext (macOS 26)

**Project build rules:** [docs/BUILD-RULES.md](../../../docs/BUILD-RULES.md) — target **macOS 26**, **WhateverGreen absent**.

Bundle IDs:
- `dev.metalgpudrivers.RaphaelIGPU` — PCI controller (DID-only `0x164E1002`)
- `dev.metalgpudrivers.RaphaelFB` — GOP/linear `IOFramebuffer` (optional second kext)

Neither matches the RTX 5080 or any other VGA device.

## Product environment

| Field | Rule |
|---|---|
| OS | **macOS 26** (ship pin). Sequoia dumps under `docs/traces/` are historical bring-up only. |
| WhateverGreen | **Absent.** Do not install WEG for this driver. Do not pass `-wegnoegpu` / `-wegnoigpu`. |
| Lilu graphics plugins | Not part of the product stack. |
| Discrete GPU | May be present; we never match its DID. Connector-driven display policy still applies. |

## Two kexts (why)

`RaphaelFramebuffer` subclasses `IOFramebuffer`, so that binary must list `com.apple.iokit.IOGraphicsFamily`. On modern macOS that family lives in the **System** kernel collection. OpenCore prelinks third-party kexts into the **Boot** kernel collection. If `IOGraphicsFamily` is not already in the collection being linked, prelink injection fails with:

```
OCAK: Dependency com.apple.iokit.IOGraphicsFamily was not found for kext dev.metalgpudrivers.RaphaelIGPU
OC: Prelinked injection RaphaelIGPU.kext (RaphaelIGPU.kext) - Invalid Parameter
```

That failure was first seen on Sequoia (`docs/traces/sequoia-7950x3d/opencore-inject-2026-09-07.txt`) while Lilu/WEG still injected — historical context only. Re-validate inject on **macOS 26 without WEG**.

`RaphaelIGPU.kext` therefore depends only on `IOPCIFamily` + KPIs (already present in that Boot KC log). It can attach, map BARs, parse ATOM, and publish connector nubs **without** taking the screen.

`RaphaelFB.kext` still needs `IOGraphicsFamily` in the **same** kernel collection as itself. This file does not document SIP, OpenCore, AuxKC, or how to put `IOGraphicsFamily` there.

## What RaphaelIGPU.kext does

- Attaches `RaphaelController` beside Apple `AMDSupport` (separate `IOMatchCategory=RaphaelHW`).
- Enumerates HDMI / DP / USB-C from ATOM `displayObjectInfo` when a PCI option ROM is mapped; otherwise assumes HDMI+DP (+USB-C offline).
- Lab ACPI VFCT VBIOS for the 7950X3D Sequoia dump (parsed offline, blob not in git): **HDMI-A + DP only** — see `docs/traces/sequoia-7950x3d/vfct-atom-connectors.txt`. USB-C was not an ATOM connector object on that image.
- Extra heads stay **offline** until DCN 3.1.5 modeset exists. `raphael_force_all=1` only matters once `RaphaelFB.kext` is actually running.
- Does **not** implement Metal, QE/CI, VCN, or a GFX10.3 ring. Does **not** replace `IONDRVFramebuffer` by itself.

## What RaphaelFB.kext does (when it is in the kernel)

- Wins `IOFramebuffer` vs `IONDRVFramebuffer` (probe 100000 vs 20000) and wraps the GOP/linear aperture for the boot head.
- Extra HDMI/DP nubs remain offline until DCN; they still share the GOP buffer — not independent scanout.

## Darwin build

On a Mac with the **macOS 26** SDK / Kernel.framework headers:

```
make -C ihv/amd-rdna2-igpu kext
```

Output:
- `ihv/amd-rdna2-igpu/build/RaphaelIGPU.kext`
- `ihv/amd-rdna2-igpu/build/RaphaelFB.kext`

Portable check (Linux or macOS, no SDK):

```
make test
```

## Adding it to an existing config

Place **`RaphaelIGPU.kext`** on the boot-time kext load path used for this project’s drivers.

**Do not** add WhateverGreen for this product. **Do not** pass `-wegnoegpu`. **Do not** match or disable a companion discrete GPU (e.g. RTX 5080) from this slot.

Do not expect a picture change from `RaphaelIGPU.kext` alone — `IONDRVFramebuffer` keeps the GOP head until `RaphaelFB` attaches. Success is `kextstat` / `ioreg` showing `RaphaelController` on `IGPU@0` next to `AMDSupport`.

`RaphaelFB.kext` is the GOP wrap. It cannot prelink unless `IOGraphicsFamily` is in that same kernel collection.

This file does not document SIP, OpenCore, AuxKC, or unsigned-kext loading.

## Boot-args (optional)

| Arg | Effect |
|---|---|
| `raphael_width` / `raphael_height` | GOP wrap mode in `RaphaelFB.kext`; default **3840×2160** from the Sequoia 4K dump (re-measure on macOS 26). |
| `raphael_force_all=1` | Mark extra HDMI/DP nubs online (still no second DCN pipe). Only observed if `RaphaelFB.kext` attached. |
| `raphael_metal=1` | Advertise `MetalPluginName=RaphaelMTLDriver` on the controller kext. **Leave off.** The plugin is a stub; Metal.framework will fail or crash if it dlopens it. |

## After load — what to capture

**Controller-only (current Boot KC path):** `kextstat` shows `dev.metalgpudrivers.RaphaelIGPU`. `ioreg` under `IGPU@0` shows `RaphaelController` (and connector nubs). `.Display_boot` / `IONDRVFramebuffer` should still own the screen. Apple `AMDSupport` may remain as a sibling. Companion dGPU trees (e.g. RTX 5080 `GFX0`) must be unchanged. Confirm **WEG is not loaded**.

**If GOP wrap is actually in the kernel:** `dev.metalgpudrivers.RaphaelFB` is loaded, and `RaphaelFramebuffer` replaces `.Display_boot` / `IONDRVFramebuffer`. Picture should still be the unaccelerated GOP desktop at the firmware mode — not extra heads, not Metal.

If the picture is wrong after wrap (tiled, wrong size, black after WindowServer), GOP mode likely does not match firmware — report `raphael_width`/`raphael_height` and which physical jack is cabled.
