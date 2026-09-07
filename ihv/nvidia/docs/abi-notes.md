# Nvidia ABI notes

Observed RPC class IDs, open-gpu-doc headers opened, and macOS IORegistry traces.

**Status:** Phase 0 freeze + N1 enumerate kext. GSP RPC table empty until N1.4.

## Lab IORegistry (Sequoia 15.7.9 / 24G830)

RTX 5080 (`10de:2c02`) dual-GPU Hackintosh: [`traces/sequoia-rtx5080/`](traces/sequoia-rtx5080/), summary in [`boards.md`](boards.md).

| Observation | Value |
|---|---|
| Nub | `GFX0@0` under `GPP0@1,1` |
| Link | x16 @ 32 GT/s |
| FB child | `IONDRVFramebuffer` (Apple), probe 20000 |
| Vendor stack | **absent** (No Kext Loaded) |
| Co-GPU | `RaphaelController` on `1002:164E` (`RaphaelPhase=R1-enumerate`) |
| HDAU | `10de:22e9` @ `HDAU@0,1` |

## Headers opened

| Header / doc | Purpose | Date |
|---|---|---|
| open-gpu-kernel-modules README Compatible GPUs | Confirm DID `2C02` = RTX 5080 | 2026-09-07 |
| Nova GB20x firmware patch notes | `.fwsignature_gb20x` naming | 2026-09-07 |

No Ampere/Blackwell compute class headers opened yet for submit — stop before inventing QMD/RPC IDs.

## UNKNOWN list (active)

1. **macOS GSP firmware redistribution** — blocks N1.4
2. GSP graphics vs compute feature set reachable from a macOS host RPC client
3. IOGPU vs IOAccelContext2 selectors on Sequoia/Tahoe Intel (trace AMD X6000 when available)
4. `CompilerPluginInterface` vs in-bundle AIR→SASS
5. Exact GB203 compute class + QMD version — open matching open-gpu-doc header before N3
6. Display-engine / NVDisplay class for GB203 — open header before N2 modeset
7. Honest minimum `MTLGPUFamily` for Blackwell
8. Cross-device IOSurface without CPU (5080 VRAM ↔ Raphael UMA)
9. AGDC / GPUWrangler requirements for secondary PCIe FB while iGPU owns console
10. Whether Sequoia 15.7 vs Tahoe 26 changes IOFramebuffer KPI for third-party kexts
