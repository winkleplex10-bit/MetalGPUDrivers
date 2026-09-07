# Building RaphaelIGPU.kext (Sequoia)

Bundle ID: `dev.metalgpudrivers.RaphaelIGPU`. Match is **DID-only** `0x164E1002` (Ryzen 7000 Raphael iGPU). It does not match the RTX 5080 or any other VGA device.

## What this first kext does

- Attaches `RaphaelController` beside Apple `AMDSupport` (separate `IOMatchCategory=RaphaelHW`).
- Wins `IOFramebuffer` vs `IONDRVFramebuffer` (probe 100000 vs 20000) and wraps the GOP/linear aperture for the boot head.
- Enumerates HDMI / DP / USB-C from ATOM `displayObjectInfo` when VBIOS is mapped; otherwise assumes HDMI+DP (+USB-C offline).
- Extra heads stay **offline** until DCN 3.1.5 modeset exists. `raphael_force_all=1` marks extra HDMI/DP online but they still share the GOP buffer — not independent scanout.
- Does **not** implement Metal, QE/CI, VCN, or a GFX10.3 ring. System Information may still say acceleration is missing until later phases.

## Darwin build

On a Mac with the Sequoia SDK / Kernel.framework headers:

```
make -C ihv/amd-rdna2-igpu kext
```

Output: `ihv/amd-rdna2-igpu/build/RaphaelIGPU.kext`

Portable check (Linux or macOS, no SDK):

```
make test
```

## Adding it to an existing config

Place the kext in the **same boot-time kext load path** you already use for Lilu / WhateverGreen so IOKit matching runs before `IONDRVFramebuffer` owns `IOFramebuffer`. Late `kextload` after NDRV has attached cannot steal that category.

Do not remove WhateverGreen. Do not pass `-wegnoegpu` for this backend. Do not match or disable the Nvidia card.

This file does not document SIP, OpenCore, AuxKC, or unsigned-kext loading.

## Boot-args (optional)

| Arg | Effect |
|---|---|
| `raphael_width` / `raphael_height` | GOP wrap mode; default **3840×2160** from the Sequoia 4K dump. Set these if firmware booted a different mode. |
| `raphael_force_all=1` | Mark extra HDMI/DP nubs online (still no second DCN pipe). |
| `raphael_metal=1` | Advertise `MetalPluginName=RaphaelMTLDriver`. **Leave off.** The plugin is a stub; Metal.framework will fail or crash if it dlopens it. |

## After load — what to capture

If attach works, `kextstat` shows `dev.metalgpudrivers.RaphaelIGPU`, and `ioreg` under `IGPU@0` shows `RaphaelController` + `RaphaelFramebuffer` instead of `.Display_boot` / `IONDRVFramebuffer`. Apple `AMDSupport` may remain as a sibling. The RTX 5080 `GFX0` tree must be unchanged.

If the picture is wrong (tiled, wrong size, black after WindowServer), GOP mode likely is not 4K — report `raphael_width`/`raphael_height` that match the firmware mode, and which physical jack is cabled.
