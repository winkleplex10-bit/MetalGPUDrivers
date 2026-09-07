# Building RaphaelIGPU.kext (Sequoia)

Bundle IDs:
- `dev.metalgpudrivers.RaphaelIGPU` — PCI controller (DID-only `0x164E1002`)
- `dev.metalgpudrivers.RaphaelFB` — GOP/linear `IOFramebuffer` (optional second kext)

Neither matches the RTX 5080 or any other VGA device.

## Two kexts (why)

`RaphaelFramebuffer` subclasses `IOFramebuffer`, so that binary must list `com.apple.iokit.IOGraphicsFamily`. On Sequoia that family lives in the **System** kernel collection. OpenCore prelinks third-party kexts into the **Boot** kernel collection. If `IOGraphicsFamily` is not already in the collection being linked, prelink injection fails with:

```
OCAK: Dependency com.apple.iokit.IOGraphicsFamily was not found for kext dev.metalgpudrivers.RaphaelIGPU
OC: Prelinked injection RaphaelIGPU.kext (RaphaelIGPU.kext) - Invalid Parameter
```

That is the 7 Sep 2026 lab log (`docs/traces/sequoia-7950x3d/opencore-inject-2026-09-07.txt`). Lilu / WhateverGreen still injected **Success**. The kext never entered the kernel, so the desktop stayed on `IONDRVFramebuffer`. `OCPE: PeCoff has no SecDir - Invalid Parameter` on EFI drivers is a separate unsigned-PE message and is not this failure.

`RaphaelIGPU.kext` therefore depends only on `IOPCIFamily` + KPIs (already present in that Boot KC log). It can attach, map BARs, parse ATOM, and publish connector nubs **without** taking the screen.

`RaphaelFB.kext` still needs `IOGraphicsFamily` in the **same** kernel collection as itself. This file does not document SIP, OpenCore, AuxKC, or how to put `IOGraphicsFamily` there.

## What RaphaelIGPU.kext does

- Attaches `RaphaelController` beside Apple `AMDSupport` (separate `IOMatchCategory=RaphaelHW`).
- Enumerates HDMI / DP / USB-C from ATOM `displayObjectInfo` when a PCI option ROM is mapped; otherwise assumes HDMI+DP (+USB-C offline).
- Lab ACPI VFCT VBIOS for this 7950X3D (parsed offline, blob not in git): **HDMI-A + DP only** — see `docs/traces/sequoia-7950x3d/vfct-atom-connectors.txt`. USB-C was not an ATOM connector object on that image.
- Extra heads stay **offline** until DCN 3.1.5 modeset exists. `raphael_force_all=1` only matters once `RaphaelFB.kext` is actually running.
- Does **not** implement Metal, QE/CI, VCN, or a GFX10.3 ring. Does **not** replace `IONDRVFramebuffer` by itself.

## What RaphaelFB.kext does (when it is in the kernel)

- Wins `IOFramebuffer` vs `IONDRVFramebuffer` (probe 100000 vs 20000) and wraps the GOP/linear aperture for the boot head.
- Extra HDMI/DP nubs remain offline until DCN; they still share the GOP buffer — not independent scanout.

## Darwin build

On a Mac with the Sequoia SDK / Kernel.framework headers:

```
make -C ihv/amd-rdna2-igpu kext
# or:
make -C ihv/amd-rdna2-igpu xcodebuild
```

Xcode projects (one per kext):
- `ihv/amd-rdna2-igpu/RaphaelIGPU.xcodeproj`
- `ihv/amd-rdna2-igpu/RaphaelFB.xcodeproj`

Output:
- Makefile: `ihv/amd-rdna2-igpu/build/RaphaelIGPU.kext`, `RaphaelFB.kext`
- Xcode: `ihv/amd-rdna2-igpu/build/xcode/Release/*.kext`

Portable check (Linux or macOS, no SDK):

```
make test
```

## Adding it to an existing config

Place **`RaphaelIGPU.kext`** in the **same boot-time kext load path** you already use for Lilu / WhateverGreen.

Do not expect a picture change from `RaphaelIGPU.kext` alone — `IONDRVFramebuffer` keeps the GOP head. Success is `kextstat` / `ioreg` showing `RaphaelController` on `IGPU@0` next to `AMDSupport`.

`RaphaelFB.kext` is the GOP wrap. It cannot prelink unless `IOGraphicsFamily` is in that same kernel collection. Do not remove WhateverGreen. Do not pass `-wegnoegpu`. Do not match or disable the Nvidia card.

This file does not document SIP, OpenCore, AuxKC, or unsigned-kext loading.

## Boot-args (optional)

| Arg | Effect |
|---|---|
| `raphael_width` / `raphael_height` | GOP wrap mode in `RaphaelFB.kext`; default **3840×2160** from the Sequoia 4K dump. Firmware GOP on 7 Sep 2026 was 3840×2160, 4 BPP, FB size `0x1FA4000`. |
| `raphael_force_all=1` | Mark extra HDMI/DP nubs online (still no second DCN pipe). Only observed if `RaphaelFB.kext` attached. |
| `raphael_metal=1` | Advertise `MetalPluginName=RaphaelMTLDriver` on the controller kext. **Leave off.** The plugin is a stub; Metal.framework will fail or crash if it dlopens it. |

## After load — what to capture

**Controller-only (current Boot KC path):** `kextstat` shows `dev.metalgpudrivers.RaphaelIGPU`. `ioreg` under `IGPU@0` shows `RaphaelController` (and connector nubs). `.Display_boot` / `IONDRVFramebuffer` should still own the screen. Apple `AMDSupport` may remain as a sibling. The RTX 5080 `GFX0` tree must be unchanged.

**If GOP wrap is actually in the kernel:** `dev.metalgpudrivers.RaphaelFB` is loaded, and `RaphaelFramebuffer` replaces `.Display_boot` / `IONDRVFramebuffer`. Picture should still be the unaccelerated GOP desktop at the firmware mode — not extra heads, not Metal.

If the picture is wrong after wrap (tiled, wrong size, black after WindowServer), GOP mode likely is not 4K — report `raphael_width`/`raphael_height` that match the firmware mode, and which physical jack is cabled.
