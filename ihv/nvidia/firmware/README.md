# Nvidia GSP firmware

GSP images are obtained at **build time**, not committed to git unless a macOS redistribution grant is documented.

## Sources (study only)

- [NVIDIA open-gpu-kernel-modules](https://github.com/NVIDIA/open-gpu-kernel-modules) — GSP boot, RPC queues
- [GSP firmware chapter](https://download.nvidia.com/XFree86/Linux-x86_64/575.57.08/README/gsp.html) — linux-firmware naming (`gsp_*.bin`)

## License status

**UNKNOWN** — linux-firmware redistribution ≠ macOS grant. Do not commit blobs until resolved.

## Implementation

Source for GSP boot + RPC client lives under `gsp/`. See CURSOR-START-NVIDIA §4–§5.
