# coexistence.md — discrete GPU + WhateverGreen policy

**Project rules:** [docs/BUILD-RULES.md](../../../docs/BUILD-RULES.md).

**Product policy (macOS 26):**

| Item | Rule |
|---|---|
| WhateverGreen | **Absent** — not part of the supported product stack |
| Discrete GPUs | May be present; this IHV never matches their DIDs |
| Primary display | **Connector-driven** |

Do **not** require WEG, do **not** require `-wegnoegpu`, and do **not** depend on WEG patch sites.

## Historical baseline (Sequoia 15.7.8) — archaeology only

The lab dump that froze Raphael identity had WEG loaded. That is **bring-up evidence**, not the ship config.

| Field | Value (historical) |
|---|---|
| Lilu | 1.7.2 |
| WhateverGreen | **1.7.1d7** (`as.vit9696.laobamac.WhateverGreen`) — **not** required for product |
| Discrete GPU | **RTX 5080** `10de:2c02` (MSI `1462:5315`) — PCI only, no Metal |
| iGPU | Raphael `1002:164E` rev C9 — **main display**, IONDRVFramebuffer |
| `-wegnoegpu` | Not used (iGPU is main) |

Re-validate attach and isolation on **macOS 26 with WEG absent**.

## Implications for the kext

| Fact | Design consequence |
|---|---|
| Product has no WEG | No Lilu plugin; no reliance on AGDP/connector spoof helpers |
| Discrete GPU may still be installed | DID-only match `0x164E1002`; never class-match all VGA |
| iGPU can be main display without WEG | G2 replaces/augments NDRV on APU connectors |
| RTX 5080 has no Metal | Dual-`MTLDevice` acceptance needs an AMD dGPU later; 5080 only tests “leave `10de:*` alone” |
| Apple `AMDSupport` matched iGPU | Expect AMDSupport sibling until we own FB/accel categories carefully |
| Both devices `IOName = display` | **Never** match on `IONameMatch=display` alone |

## Checklist

- [x] Unsupported dGPU present; iGPU still enumerates (Sequoia history)
- [ ] **macOS 26, WEG absent:** controller attaches; `GFX0` undisturbed
- [ ] After our FB attaches: APU monitor still works; companion dGPU path undisturbed
- [ ] `MTLCopyAllDevices` dual listing with AMD dGPU (not applicable with 5080 alone)

## Non-goals

- No Nvidia / RTX 5080 acceleration in this slot
- No installing or forking WhateverGreen for product
- No OpenCore load recipes in this file
- No X6000 DID spoof / personality injection for `0x164E` (see plan §0.2)
