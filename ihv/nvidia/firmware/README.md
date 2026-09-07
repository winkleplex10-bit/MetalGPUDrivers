# Nvidia GSP firmware

GSP images are obtained at **build time**, not committed to git unless a macOS redistribution grant is documented.

## Lab chip (RTX 5080 / GB203)

- Architecture class: **Blackwell GB20x** (Nova/open-rm: `.fwsignature_gb20x`)
- Open kernel modules **require** GSP on Blackwell; proprietary `nvidia.ko` flavor is unsupported for this generation
- linux-firmware / driver packages name blobs under versioned trees (e.g. `nvidia/<ver>/…` or arch-specific `gsp_*.bin`); exact macOS load path is TBD in `firmware/gsp/` source

Cited: [open-gpu-kernel-modules](https://github.com/NVIDIA/open-gpu-kernel-modules) Compatible GPUs (`2C02`); Nova Hopper/Blackwell firmware patches (`.fwsignature_gb20x`); [GSP firmware chapter](https://download.nvidia.com/XFree86/Linux-x86_64/575.57.08/README/gsp.html).

## License status

**UNKNOWN** — linux-firmware redistribution ≠ a macOS grant. Do not commit blobs. N1.4 (GSP boot) **stops here** until resolved for this project.

## Implementation plan

1. N1 enumerate kext maps BARs only (`NvidiaController`) — **no firmware load**
2. When license is clear: build-time copy of matching open-rm GSP image into a non-git path; RPC client under `firmware/gsp/`
3. Log observed RPC class IDs in [docs/abi-notes.md](../docs/abi-notes.md); never guess
