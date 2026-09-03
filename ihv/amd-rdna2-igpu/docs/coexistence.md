# coexistence.md — WhateverGreen + discrete GPU

**Product policy:** this IHV must coexist with stock WhateverGreen and with dedicated GPUs. Unlike NootedRed, we do **not** require removing WEG or disabling the dGPU.

**Primary display:** connector-driven — whichever GPU has the active monitor cable(s) owns that head.

## R0 baseline (fill in)

| Field | Value |
|---|---|
| WhateverGreen version | UNKNOWN |
| Lilu version | UNKNOWN |
| Discrete GPU VID:DID | UNKNOWN |
| Discrete stack | Apple X6000 / other — fill |
| `-wegnoegpu` used? | **Must be no** for dual-GPU acceptance |
| WEG removed? | **Must be no** for acceptance |
| APU physical ports | UNKNOWN — list |
| dGPU physical ports | UNKNOWN — list |

## Checklist

- [ ] WEG loaded at boot with our kext absent; dGPU IORegistry healthy
- [ ] After our `match/` attaches: dGPU nub still owned by Apple/WEG path, not us
- [ ] Monitor on APU ports → our FB can become primary (R2+)
- [ ] Monitor on dGPU ports only → dGPU remains primary; no black screen from us
- [ ] Monitors on both → both FBs active; no forced exclusive primary
- [ ] `MTLCopyAllDevices` dual listing verified at R5+
- [ ] Measured: does WEG touch DID `0x164E`? (CURSOR-START §10.9)

## Non-goals

- Do not vendor or fork WhateverGreen
- Do not document OpenCore kext-load recipes here
- Do not treat Nvidia acceleration as this slot’s problem — only “must not break iGPU attach”
