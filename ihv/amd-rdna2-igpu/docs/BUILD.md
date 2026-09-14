# Building RaphaelIGPU.kext / RaphaelFB.kext (macOS 26 Tahoe)

Binding: [docs/BUILD-RULES.md](../../../docs/BUILD-RULES.md) — **macOS 26** SDK, **WhateverGreen absent**. This file does not document SIP, OpenCore, AuxKC, or unsigned-kext loading.

Bundle IDs:
- `dev.metalgpudrivers.RaphaelIGPU` — PCI controller (DID-only `0x164E1002`)
- `dev.metalgpudrivers.RaphaelFB` — GOP/linear `IOFramebuffer`

Neither matches the RTX 5080 or any other VGA device.

## Two kexts

`RaphaelFramebuffer` subclasses `IOFramebuffer`. That class lives in `com.apple.iokit.IOGraphicsFamily`. `RaphaelFB.kext` **must** list that family in `OSBundleLibraries` so the linker can resolve `IOFramebuffer::gMetaClass` (`__ZN13IOFramebuffer10gMetaClassE`). v0.2.0 omitted it; `kmutil` then failed with “Cannot find symbol for metaclass pointed to by `__ZN15IOFBLinearShell10superClassE`”. v0.2.1 lists the family again.

GOP **geometry** comes from `IOPlatformExpert::getConsoleInfo`. AuxKC cannot bind pexpert internals `_PE_current_console` or `_PE_state` (v0.2.1 failed there). `PE_parse_boot_argn` remains (kpi.unsupported).

On Tahoe, `getConsoleInfo` `v_baseAddr` was **not** a usable physical (`0x10000000001`) while 3840×2160 / pitch 15360 / 33177600 bytes were correct. v0.2.3 describes the GOP range from a PCI BAR already on the iGPU nub (`getDeviceMemoryWithIndex`). Still no `setMemoryEnable` / `mapDeviceMemory`. `raphael_swap_rb=1` swaps R/B masks if the picture is channel-swapped after the BAR wrap.

`RaphaelIGPU.kext` does **not** list `IOGraphicsFamily` (controller only, `IOPCIFamily` + KPIs). Load the controller bundle before RaphaelFB (FB lists `dev.metalgpudrivers.RaphaelIGPU`).

This file does not document SIP, OpenCore, AuxKC, or unsigned-kext loading.

## What RaphaelIGPU.kext does

- Attaches `RaphaelController` beside Apple `AMDSupport` (`IOMatchCategory=RaphaelHW`).
- Default: no PCI memory / BAR map (`raphael_map=1` hung a live GOP head). ATOM/VFCT fallback HDMI+DP (2 connectors). USB-C only if `raphael_force_all=1`.
- Identity properties are `RaphaelVendorId` / `RaphaelDeviceId` / `RaphaelModel` (not PCI `vendor-id` / `model`).
- Does **not** implement Metal, QE/CI, VCN, or a GFX10.3 ring.

## What RaphaelFB.kext does (when it is in the kernel)

- Wins `IOFramebuffer` vs `IONDRVFramebuffer` (probe 100000 vs 20000) on DID `0x164E` only, **if matching still runs**.
- Geometry from `getConsoleInfo`. Aperture from a PCI BAR on the iGPU nub when console `v_baseAddr` is not a page-aligned physical. Does **not** call `setMemoryEnable` / `mapDeviceMemory`.
- Picture stays the unaccelerated GOP desktop. Not extra heads, not Metal.
- `raphael_fb=0` skips probe so IONDRV stays.

**Tahoe lab (0.2.2):** boot-time load wrapped `VGA@0` (`RaphaelPhase=R2-gop-wrap`, WindowServer `fb0=/RaphaelFramebuffer`). Load after IONDRV already owns `IOFramebuffer` on that nub does not displace `.Display_boot`. Capture `ioreg` either way.

Current kext version in tree: **0.2.10**. 0.2.8 on-box boot 13 Sep 2026: one GPINT + HUBP blank/unblank (picture came back). **0.2.9** left `V_TOTAL+1` live on the lab `raphael_dcn_modeset=1` path — black screen, never recovered (not Metal). **0.2.10** uses the 0.2.8 pipe on that arg. **Sleep/shutdown/restart parked** — do not add PCI PM or HUBP blank from `setPowerState`. Firmware in IGPU Resources. Lab AuxKC path is `/Library/Extensions` (identity fact only — this file does not document how to load it). If 0.2.9 is still installed and the console is black, boot without `raphael_dcn_modeset=1` or with `raphael_fb=0` until 0.2.10 replaces it.

## Darwin build

```
make -C ihv/amd-rdna2-igpu kext
```

Output:
- `ihv/amd-rdna2-igpu/build/RaphaelIGPU.kext`
- `ihv/amd-rdna2-igpu/build/RaphaelFB.kext`

Portable check:

```
make test
```

Firmware (Darwin `make kext` fetches if missing):

```
make fetch-firmware
```

Blobs land in `firmware/cache/` and `build/RaphaelIGPU.kext/Contents/Resources/` (never git). LICENSE.amdgpu is copied next to them.

Current lab: **Tahoe, no WhateverGreen**. Do not require Lilu/WEG, do not pass `-wegnoegpu`, and do not match or disable the Nvidia card. If WEG is loaded on another machine, the same DID-only match still applies.

## Boot-args (optional)

| Arg | Effect |
|---|---|
| `raphael_fb=0` | Do not attach `RaphaelFramebuffer`; IONDRV keeps GOP. |
| `raphael_width` / `raphael_height` | Override GOP mode advertised to IOGraphics. Default is `getConsoleInfo`. |
| `raphael_swap_rb=1` | Swap R/B component masks (channel-swap / leftover tint after BAR wrap). |
| `raphael_map=1` | Map iGPU BAR0/1. **Leave off.** |
| `raphael_dcn_probe=1` | **Dangerous opt-in.** After GOP wrap, map **BAR5 only** (512KB DCN MMIO, PCI cfg 0x24) and log phys/size/map. Read-only HPD (named `DC_HPD_SENSE` / `RX`) and OTG0–3 dump. Does not toggle Memory Enable, does not map BAR0. Default unset = GOP wrap. |
| `raphael_dcn_dump=1` | Implies `raphael_dcn_probe`. Same BAR5 map; dumps live OTG/ODM pipe after HPD. |
| `raphael_dcn_modeset=1` | After GOP wrap: map BAR5, load `dcn_3_1_5_dmcub.bin` from kext Resources, GPINT when ENABLE+mailbox_rdy (do **not** abort on `dal_fw=0`), **reaffirm** live OTG 3840×2160 (physical **DP**, OTG0/HPD2), then HUBP blank/unblank (**same GOP totals**). Default unset = no DCN write. **Sleep/shutdown/restart parked** — do not join PCI PM or blank HUBP from `setPowerState`. |
| `raphael_dcn_vtotal=1` | Opt-in OTG-only `V_TOTAL+1` probe. Requires `raphael_dcn_modeset=1`. Verifies while HUBP is blanked, **restores GOP totals before unblank**. Can still drop a DP sink. **Do not add this to the lab boot-arg until 0.2.10 desktop is confirmed.** |
| `raphael_force_all=1` | HDMI+DP+USB-C fallback nubs (this SKU’s VFCT has no USB-C). |
| `raphael_metal=1` | Advertise `MetalPluginName=RaphaelMTLDriver`. **Leave off.** |

## After load — what to capture

**Controller:** `kextstat` shows `dev.metalgpudrivers.RaphaelIGPU`. `ioreg` under `VGA@0` (`pci1002,164e`, parent `GP17@8,1`) shows `RaphaelController`. 5080 path unchanged.

**GOP wrap:** `RaphaelFramebuffer` on that same nub, `RaphaelPhase=R2-gop-wrap`, `connector-kind=DP` (boot head DP index 1; VFCT HDMI-A name disagrees). iGPU `.Display_boot` gone. `kConnectionFlags` must not be BuiltIn on HDMI/DP. Unaccelerated 4K. Remaining `IONDRVFramebuffer` is the 5080.

**BAR5 probe (only with `raphael_dcn_probe=1` or `raphael_dcn_dump=1`):** log `RaphaelController: BAR5 phys=… size=… map=1`. Then HPD named bits (`SENSE=` not UNKNOWN) and `OTG0`…`OTG3` / `DCN live pipe OTG…`. Hang after `HDMI index 0 boot=1` is BAR5, not GOP attach. Default boot without `dcn_modeset`/`probe`/`dump` does not map BAR5.

**`raphael_dcn_modeset=1` (0.2.10 = 0.2.8 pipe):** BAR5 `phys=0xdd600000 size=524288 map=1`. DMCUB `ENABLE=1 SCRATCH0=0x42 dal_fw=0 mailbox_rdy=1`. `GPINT GET_FW_VERSION ok response=0x05000649` (once). OTG0 reaffirm `H_TOTAL=0xf9f V_TOTAL=0x8ad` CTL `0x11301`. Then HUBP `HUBP_BLANK_EN` blank, `IOSleep(100)`, unblank; totals still GOP. `OTG_BLANK_CONTROL` is **not** in `dcn_3_1_5_offset.h`. Controller `RaphaelPhase=R2-dcn-modeset`. `RaphaelDmubOk=Yes`. FB stays `R2-gop-wrap`. **Do not leave V_TOTAL+1 live.** **Sleep/wake/shutdown/restart parked** (GOP may stay lit). Do **not** add framebuffer power hooks.

**setDisplayMode:** GOP 3840×2160 succeeds (software if `dcn_modeset` off or handshake failed). `raphael_dcn_modeset=1` issues reaffirm then HUBP blank/unblank after DMUB GPINT (`setDisplayMode hardware 3840x2160 (OTG live DP pipe)`). Live jack is **DP** (user), not VFCT HDMI path 0. Next kext slice is a cited DP stream/MSA or DIG change that can recover the picture (plan §11), not a second head and not PM.
