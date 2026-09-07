# personalities.md — Nvidia PCI match

## Product personality (N1 enumerate)

| Key | Value |
|---|---|
| Bundle ID | `dev.metalgpudrivers.NvidiaGSP` |
| IOClass | `NvidiaController` |
| IOProviderClass | `IOPCIDevice` |
| IOPCIPrimaryMatch | `0x2c0210de` (RTX 5080) |
| IOMatchCategory | `NvidiaHW` |
| IOProbeScore | `200000` |
| IOPCITunnelCompatible | not set (internal PCIe Slot-1; prefer MSI when interrupts are added) |

## Why not class-match

`IOPCIClassMatch = 0x03000000&0xff000000` would also match the Raphael iGPU (`1002:164E`) and any future VGA. Exact DID only.

## Categories

| Category | Owner | Notes |
|---|---|---|
| `NvidiaHW` | `NvidiaController` | Enumerate / GSP / submit parent |
| `IOFramebuffer` | still `IONDRVFramebuffer` on this card | Phase N2 will publish our FB subclass; until then do not steal |

## Boot-args

| Arg | Effect |
|---|---|
| `-nvoff` | Do not attach |
| `nv_map=0` | Attach without retaining BAR memory descriptors |

## Audio / other functions

HDAU `10de:22e9` @ `HDAU@0,1` is class `0x040300`. Out of scope for N1–N2 display.
