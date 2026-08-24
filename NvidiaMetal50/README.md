# NvidiaMetal50

Unaccelerated NVIDIA display for macOS on Intel / Hackintosh. First milestone: **the card must not prevent boot**, and the UEFI GOP framebuffer must be enough for the **installer and a software-rendered desktop**. No Metal, no GSP, no mode changes.

Apple leftovers `NVDAStartup` / `NVDAResman` / `GeForce.kext` match every NVIDIA VGA device (`0x10DE`, class `0x03`) with probe score 100000. On Maxwell and newer they hang or panic. This kext matches the same devices at score **500000**, holds the `IOFramebuffer` category, and never programs the GPU.

## What you get

| Situation | Result |
|---|---|
| GTX/RTX is the UEFI boot display | Installer + desktop at the firmware resolution, unaccelerated |
| iGPU is the boot display, NVIDIA is secondary | NVIDIA is claimed so leftover Kepler kexts cannot attach; display stays on the iGPU |
| Leftover `NVDAStartup` still installed | This kext wins the match; block those kexts anyway (see [docs/INSTALL.md](docs/INSTALL.md)) |

## Not in this kext

GSP bring-up, NVKMS, Metal, resolution changes, HDMI audio, VideoToolbox. Those come later under `kext/NvidiaRM` and userspace bundles. Do not poke BAR0.

## Build

x86_64, macOS 10.15+ SDK:

```bash
cd NvidiaMetal50
make
```

Output: `build/NvidiaMetal50.kext`

## Load

Hackintosh: inject with OpenCore (`Kernel > Add`) and block Kepler NVIDIA kexts (`Kernel > Block`). SIP / AMFI must allow unsigned kexts. Full steps: [docs/INSTALL.md](docs/INSTALL.md).

## Boot-args

| Arg | Effect |
|---|---|
| `-nvfboff` | Do not attach (legacy NV kexts can match again) |
| `-nvfbclaim` | Claim the PCI device only; do not publish a display |
| `-nvfbforce` | Use the boot GOP even if its address is not inside a BAR (sysmem bounce buffer) |

Logs: `NvidiaMetal50:` in `log show --predicate 'process == "kernel"' --last boot` or `-v` boot.

## Tree

```
kext/NvidiaFB/     GOP IOFramebuffer (this milestone)
tools/             disable leftover Kepler kexts; IORegistry dump
docs/              OpenCore install, AGDP, lab notes
```

`../open-gpu-kernel-modules` stays the RM/GSP reference for the next milestone. Do not copy Apple binaries or GSP firmware into this folder.
