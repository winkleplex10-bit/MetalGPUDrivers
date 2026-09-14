# coexistence.md — discrete GPU (WhateverGreen absent)

**Product policy ([BUILD-RULES.md](../../../docs/BUILD-RULES.md)):** **WhateverGreen is absent.** This IHV must **not** load, patch, fork, or depend on WEG, Lilu graphics plugins, `-wegnoegpu`, or a disabled dGPU. Narrow DID match only. If WEG is present on another install, do not conflict with it.

**Current lab (2026-09-10):** **macOS 26 Tahoe, WhateverGreen absent.** Sequoia WEG measurements below are historical.

**Primary display:** connector-driven.

## Current lab (Tahoe)

| Field | Value |
|---|---|
| WhateverGreen | **Not loaded** |
| Lilu | Loaded on Tahoe (1.7.2) — not a dependency of this IHV |
| Discrete GPU | **RTX 5080** `10de:2c02` (MSI `1462:5315`) — PCI inventory; no Metal. Nvidia-slot notes: [`ihv/nvidia/docs/boards.md`](../../nvidia/docs/boards.md) |
| iGPU | Raphael `1002:164E` rev C9 — nub **`VGA@0`**. **0.2.10:** `RaphaelFramebuffer` boot FB (`connector-kind=DP`); AMDSupport sibling; controller `R2-dcn-modeset` / `RaphaelDmubOk=Yes`. 5080 IONDRV untouched. |
| `-wegnoegpu` | Not used |

Without WEG there are no WEG AGDP/connector patches. Display is our GOP wrap (or IONDRV if we are not in the kernel in time) plus Apple `AMDSupport`. The 5080 is still **do not match**.

## Historical baseline (Sequoia 15.7.8)

| Field | Value |
|---|---|
| Lilu | 1.7.2 |
| WhateverGreen | 1.7.1d7 (`as.vit9696.laobamac.WhateverGreen`) |
| iGPU | Raphael `1002:164E` rev C9 — **main display**, IONDRVFramebuffer |
| WEG removed? | No — WEG was loaded on that dump |

## Implications for the kext

| Fact | Design consequence |
|---|---|
| Tahoe lab has no WEG | Do not depend on WEG properties, Lilu plugins, or AGDP patches |
| WEG may exist on other machines | Still not a Lilu plugin; do not race WEG patch sites |
| iGPU is already main display | G2 replaces/augments NDRV on APU connectors; do not require dGPU-primary |
| RTX 5080 has no Metal | Dual-`MTLDevice` acceptance needs an AMD dGPU later; 5080 only tests “leave `10de:*` alone” |
| Apple `AMDSupport` matched iGPU (Sequoia) | Match is **DID-specific**; expect AMDSupport sibling until we own FB/accel categories carefully |
| Both devices `IOName = display` | **Never** match on `IONameMatch=display` alone |

## Checklist

- [x] Unsupported dGPU present; iGPU still enumerates and drives display (Sequoia + Tahoe)
- [x] WEG loaded with iGPU as main display (**Sequoia only** — not current lab)
- [x] Tahoe without WEG: R1/R2 on `VGA@0`; 5080 IONDRV untouched
- [x] After match attaches: only `1002:164E`, not the 5080
- [x] After FB attaches: APU 4K desktop (0.2.3 BAR wrap); 5080 path undisturbed. 0.2.4: not advertised as Internal. 0.2.7: GPINT + live DP 4K reaffirm. 0.2.8: HUBP blank/unblank recovered. 0.2.9: V_TOTAL+1 live on DP blacked the console. 0.2.10: lab modeset is 0.2.8. Not dual heads, not Metal.
- [ ] `MTLCopyAllDevices` dual listing with AMD dGPU (not applicable with 5080 alone)

## Non-goals

- No Nvidia / RTX 5080 acceleration in this slot
- No forking WhateverGreen
- No requiring WEG to be present or absent
- No OpenCore load recipes in this file
- No X6000 DID spoof / personality injection for `0x164E` (see plan §0.2)
