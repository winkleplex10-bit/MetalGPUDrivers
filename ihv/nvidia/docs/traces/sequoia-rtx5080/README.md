# Sequoia lab traces — RTX 5080 (Blackwell)

Host also runs Raphael iGPU (`1002:164E`). Dual-GPU notes: [../coexistence.md](../coexistence.md).

| File | Contents |
|---|---|
| `sw_vers.txt` | macOS product/build (15.7.9 / 24G830 as of 2026-09-07) |
| `system_profiler.txt` | SPDisplaysDataType — Nvidia `0x2c02` + AMD `0x164e`, No Kext Loaded |
| `ioreg-gfx0-rtx5080-excerpt.txt` | `GFX0@0` properties + `IONDRVFramebuffer` child |
| `kextstat-gpu.txt` | Filtered kextstat when permitted |
| `cpu.txt` | CPU / model when `sysctl` permitted |

Captured for Phase 0 freeze and N1 baseline (“card visible, vendor stack absent”).
