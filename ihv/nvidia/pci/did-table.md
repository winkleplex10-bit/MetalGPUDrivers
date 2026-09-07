# did-table.md — Nvidia VID/DID allow-list notes

VID is always **`0x10de`**. This lab freezes **one** DID for Phase 0/1.

## Frozen (this repo)

| Marketing | Arch | DID | Rev (lab) | Subsystem (lab) | Notes |
|---|---|---|---|---|---|
| GeForce RTX 5080 | Blackwell GB20x (GB203) | **`0x2c02`** | `0xA1` | `1462:5315` (MSI) | Sequoia 15.7.9; nub `GFX0@0`; open-rm Compatible GPUs. See [boards.md](../docs/boards.md). |

Source of truth for the build: [`did-allowlist.txt`](did-allowlist.txt).

## Later same slot (do not enable yet)

| Marketing | Arch | Example DID | Notes |
|---|---|---|---|
| RTX 3090 | Ampere | `0x2204` | Prefer if Ampere board appears |
| RTX 3080 | Ampere | `0x2206` | |
| RTX 3070 | Ampere | `0x2484` | |
| RTX 40xx | Ada | open-rm table | Append-only after N4 green on frozen board |

## Rules

1. Allow-list **exact** DID(s); no `IOPCIClassMatch` VGA-wide.
2. When coexisting with AMD iGPU (`1002:164E`), Nvidia personality must never claim AMD DIDs (and vice versa).
3. HDAU `0x22e9` is not a VGA DID — leave it alone.
