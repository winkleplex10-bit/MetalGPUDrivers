# coexistence.md — WhateverGreen + discrete GPU

**Product policy:** coexist with stock/fork WhateverGreen and dedicated GPUs. Do **not** require removing WEG or `-wegnoegpu`.

**Primary display:** connector-driven.

## Measured baseline (Sequoia 15.7.8)

| Field | Value |
|---|---|
| Lilu | 1.7.2 |
| WhateverGreen | **1.7.1d7** (`as.vit9696.laobamac.WhateverGreen`) |
| Discrete GPU | **RTX 5080** `10de:2c02` (MSI `1462:5315`) — PCI only, no Metal. Nvidia-slot notes: [`ihv/nvidia/docs/boards.md`](../../nvidia/docs/boards.md) |
| iGPU | Raphael `1002:164E` rev C9 — **main display**, IONDRVFramebuffer |
| `-wegnoegpu` | Not used (iGPU is main) |
| WEG removed? | No — WEG is loaded |

## Implications for the kext

| Fact | Design consequence |
|---|---|
| WEG (laobamac build) is loaded | Product must not conflict with WEG patch sites; we are not a Lilu plugin |
| iGPU is already main display | G2 replaces/augments NDRV path on APU connectors; do not require dGPU-primary |
| RTX 5080 has no Metal | Dual-`MTLDevice` acceptance needs an AMD dGPU later; 5080 only tests “leave `10de:*` alone” |
| Apple `AMDSupport` matched iGPU | Our match must be **DID-specific**; expect AMDSupport sibling until we own FB/accel categories carefully |
| Both devices `IOName = display` | **Never** match on `IONameMatch=display` alone |

## Checklist

- [x] Unsupported dGPU present; iGPU still enumerates and drives display (Sequoia)
- [x] WEG loaded with iGPU as main display
- [ ] After our `match/` attaches: confirm we claim only `IGPU@0` / `1002:164E`, not `GFX0` (kext is DID-only `0x164E1002`; measure on box)
- [ ] After our FB attaches: APU monitor still works; 5080 path undisturbed
- [ ] Measured: does this WEG build patch DID `0x164E`? (log with WEG DEBUG if needed)
- [ ] `MTLCopyAllDevices` dual listing with AMD dGPU (not applicable with 5080 alone)

## Non-goals

- No Nvidia / RTX 5080 acceleration in this slot
- No forking WhateverGreen
- No OpenCore load recipes in this file
