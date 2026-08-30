# Nvidia IHV slot — Ampere+ display + Metal

Unofficial display + Metal backend for GSP-era Nvidia GPUs (Ampere first; Ada/Blackwell later).

**Binding spec:** [docs/CURSOR-START-NVIDIA.md](../../docs/CURSOR-START-NVIDIA.md)

## Scope

- **In:** One frozen Ampere board (GA10x), VID `0x10de`, GSP firmware brokered
- **Out:** Kepler/Maxwell/Pascal fossils, Turing as Phase 1 board, bit-bang without GSP

## Layout

```
pci/          VID/DID allow-list, tunnel notes
firmware/     GSP boot + RPC client (source); blobs at build-time only
display/      NVDisplay / scanout (heads, SOR/HDMI-DP, FB glue)
submit/       GSP RPC, GPFIFO, Ampere compute classes
isa/          AIR → SASS (Ampere first)
power/        GSP-brokered clocks only
docs/         Observed RPC IDs, traces, abi notes
```

## Phase status

Not started. See CURSOR-START-NVIDIA §6 for N0–N6 acceptance criteria.
