# Nvidia IHV slot — Ampere+ display + Metal

Unofficial display + Metal backend for GSP-era Nvidia GPUs (Ampere first; Ada/Blackwell later).

**Binding spec:** [docs/CURSOR-START-NVIDIA.md](../../docs/CURSOR-START-NVIDIA.md)

## Scope

- **In:** One frozen Ampere board (GA10x), VID `0x10de`, GSP firmware brokered
- **Later same slot:** Ada / consumer Blackwell (lab-enumerated RTX 5080 DID `0x2c02` — not Phase 1)
- **Out:** Kepler/Maxwell/Pascal fossils, Turing as Phase 1 board, bit-bang without GSP

## Lab inventory

| Board | DID | Role |
|---|---|---|
| **RTX 5080** (MSI) | `10de:2c02` rev A1 | Sequoia dual-GPU lab host with Raphael iGPU; PCI + IONDRV only. Details: [docs/boards.md](docs/boards.md), [pci/did-table.md](pci/did-table.md), [docs/traces/sequoia-rtx5080/](docs/traces/sequoia-rtx5080/) |

Cross-link: AMD iGPU coexistence on the same machine lives under [`ihv/amd-rdna2-igpu/`](../amd-rdna2-igpu/).

## Layout

```
pci/          VID/DID allow-list, tunnel notes
firmware/     GSP boot + RPC client (source); blobs at build-time only
display/      NVDisplay / scanout (heads, SOR/HDMI-DP, FB glue)
submit/       GSP RPC, GPFIFO, Ampere compute classes
isa/          AIR → SASS (Ampere first)
power/        GSP-brokered clocks only
docs/         Observed RPC IDs, traces, abi notes, boards.md
```

## Phase status

Not started (Ampere Phase 1). Blackwell 5080 is inventory/traces only until Ampere GSP path is proven. See CURSOR-START-NVIDIA §6 for N0–N6 acceptance criteria.
