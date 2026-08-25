# GOP framebuffer notes

macOS WindowServer talks to `IOFramebuffer`, not to EFI. After `ExitBootServices` the GPU keeps scanning out whatever GOP last programmed. This kext publishes that buffer as a one-mode linear framebuffer.

## Why leftover Kepler kexts hang boot

`NVDAStartup` matches vendor `0x10DE` + class `0x03` at probe 100000, then creates nubs for `NVDAResman` / HALs / `GeForce.kext`. Those HALs only know Fermi/Kepler (`GF100` / `GK100`). On Maxwell–Blackwell they MMIO the wrong registers or wait forever.

`NvidiaGopFramebuffer40` uses the same match keys with probe **500000** and `IOMatchCategory = IOFramebuffer`, so only one of the two can attach.

## What this kext does not do

It does not:

- Load GSP firmware
- Map BAR0 (MMIO)
- Change resolution (GOP mode only)
- Implement Metal / IOAccelerator
- Drive HDMI audio (`class 0x04` is left alone)

## Finding the GOP buffer

Physical address, pitch, and size come from `boot_args->Video` (EFI GOP, usually VRAM). The kext checks whether that range sits inside a PCI BAR on this device:

- Yes → this card is the console (`AAPL,boot-display`)
- No → still claim the device (block NVDAStartup); `enableController` fails so WindowServer ignores it (iGPU-primary laptops)

`-nvfbforce` uses GOP even when it is not in a BAR (OpenCore DirectGopRendering bounce buffer). That is a debug/last-resort flag.

## Next milestone (not this kext)

Rehost NVIDIA RM + NVKMS (`../open-gpu-kernel-modules`, 610.57.04) behind a real modeset IOFramebuffer. That needs GSP firmware and a Darwin `os_*` layer.
