# coexistence.md — WhateverGreen + discrete GPU

**Product policy:** this IHV must coexist with stock WhateverGreen and with dedicated GPUs. Unlike NootedRed, we do **not** require removing WEG or disabling the dGPU.

**Primary display:** connector-driven — whichever GPU has the active monitor cable(s) owns that head.

## Lab evidence (Monterey)

Reporter boot: Raphael iGPU **unaccelerated** desktop on Monterey; **System Information → Graphics** showed **both** the iGPU and an **RTX 5080**.

| Implication | Action |
|---|---|
| Dual-GPU PCI recognition already works without our kext | G1 must attach **only** to Raphael; leave `10de:*` alone |
| Unaccelerated iGPU display works | G2 builds on an already-proven GOP/basic-FB boot path |
| RTX 5080 has no macOS Metal | Do not expect dual `MTLDevice` with Nvidia; dual listing acceptance uses an **Apple-supported AMD dGPU** when available |
| “5080 listed for some reason” | Normal: System Information lists GPUs from PCI/IORegistry even with no vendor Metal plugin |

## R0 baseline

| Field | Value |
|---|---|
| WhateverGreen version | UNKNOWN — fill if present on Monterey/Tahoe boots |
| Lilu version | UNKNOWN |
| Discrete GPU | **RTX 5080** (lab); AMD RX 6000-class preferred later for WEG+Metal dual-device tests |
| Discrete stack | Nvidia: PCI only / no Metal. AMD X6000+WEG: separate acceptance board or later install |
| `-wegnoegpu` used? | **Must be no** for dual-GPU acceptance |
| WEG removed? | **Must be no** when WEG is part of the EFI |
| APU physical ports | UNKNOWN — ports that drove Monterey desktop |
| dGPU physical ports | UNKNOWN — RTX 5080 ports |

## Checklist

- [x] Unsupported dGPU (RTX 5080) present while iGPU still enumerates / boots unaccelerated (Monterey)
- [ ] WEG loaded at boot with our kext absent; record whether WEG is in the EFI
- [ ] After our `match/` attaches: dGPU nub still not claimed by us
- [ ] Monitor on APU ports → our FB can become primary (R2+)
- [ ] Monitor on dGPU ports only → no black screen from us (Nvidia may still be non-accelerated)
- [ ] Monitors on both → connector-driven; no forced exclusive primary
- [ ] `MTLCopyAllDevices` dual listing verified at R5+ **with AMD dGPU** (not expected with 5080 alone)
- [ ] Measured: does WEG touch DID `0x164E`? (CURSOR-START §10.9)

## Non-goals

- Do not vendor or fork WhateverGreen
- Do not document OpenCore kext-load recipes here
- Do not implement Nvidia / RTX 5080 acceleration in this slot — only “must not break iGPU attach”
