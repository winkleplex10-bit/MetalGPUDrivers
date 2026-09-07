# Nvidia IHV slot — GSP-era display + Metal (Blackwell lab first)

Unofficial display + Metal backend for GSP-era Nvidia GPUs.

**Binding spec:** [docs/CURSOR-START-NVIDIA.md](../../docs/CURSOR-START-NVIDIA.md)

## Scope

- **Frozen lab board:** GeForce RTX 5080 (`10de:2c02`, GB203 / GB20x) — see [docs/boards.md](docs/boards.md)
- **In:** Exact DID allow-list, GSP-brokered bring-up (later), NVDisplay, AIR→SASS
- **Out:** Kepler/Maxwell/Pascal fossils, class-wide VGA match, bit-bang without GSP

CURSOR-START prefers Ampere first; this lab freezes Blackwell because it is the only discrete Nvidia present. Same open-rm IHV slot (Blackwell **requires** open-rm + GSP).

## Phase status

| Phase | Status |
|---|---|
| N0 freeze | **Done** — DID `0x2c02`, personalities, traces |
| N1 enumerate | **Kext built** — `NvidiaGSP.kext` / `NvidiaController` (BAR map + identity; no GSP) |
| N1.4 GSP boot | Blocked on macOS firmware redistrib **UNKNOWN** |
| N2+ | Not started |

## Lab inventory

| Board | DID | Role |
|---|---|---|
| **RTX 5080** (MSI) | `10de:2c02` rev A1 | Sequoia dual-GPU with Raphael iGPU; Slot-1 PCIe Gen5 x16 |

Cross-link: [docs/coexistence.md](docs/coexistence.md), [`ihv/amd-rdna2-igpu/`](../amd-rdna2-igpu/).

## Layout

```
include/      NvidiaIds.h (frozen DID)
match/        NvidiaController (N1 enumerate)
kext/         Info.plist + kmod entry
pci/          did-allowlist, personalities
firmware/     GSP notes (blobs not committed)
display/      heads / SOR / fb (empty — N2)
submit/       rpc / fifo / compute (empty — N3)
isa/          AIR → SASS (empty — N4)
docs/         boards, BUILD, coexistence, traces, abi-notes
```

## Build

```bash
cd ihv/nvidia && make kext
# or open NvidiaGSP.xcodeproj / xcodebuild -scheme NvidiaGSP
```

See [docs/BUILD.md](docs/BUILD.md).
