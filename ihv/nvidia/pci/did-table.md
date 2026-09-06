# did-table.md — Nvidia VID/DID allow-list notes

VID is always **`0x10de`**. Product Phase 1 freezes **one Ampere** DID. Later generations share the GSP IHV slot with generation-specific tables in `submit/` / `isa/`.

## Lab-observed (this repo)

| Marketing | Arch | DID | Rev (lab) | Subsystem (lab) | Notes |
|---|---|---|---|---|---|
| GeForce RTX 5080 | Blackwell (GB20x) | **`0x2c02`** | `0xA1` | `1462:5315` (MSI) | Sequoia 15.7.8 Hackintosh; nub `GFX0@0`; no Metal. See [boards.md](../docs/boards.md). |

## Phase 1 candidates (not yet frozen — copy from open-rm when board chosen)

Examples from Nvidia open-gpu-kernel-modules Compatible GPUs (do not invent; verify in upstream table before freezing):

| Marketing | Arch | Example DID |
|---|---|---|
| RTX 3090 | Ampere | `0x2204` |
| RTX 3080 | Ampere | `0x2206` |
| RTX 3070 | Ampere | `0x2484` |

## Rules

1. Allow-list **exact** DID(s); no `IOPCIClassMatch` VGA-wide.
2. Blackwell (`0x2c02` etc.) is **same IHV slot**, later board — not Phase 1 start.
3. When coexisting with AMD iGPU (`1002:164E`), Nvidia personality must never claim AMD DIDs (and vice versa).
