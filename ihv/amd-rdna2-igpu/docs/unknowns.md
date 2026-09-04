# unknowns.md — blockers for `ihv/amd-rdna2-igpu`

Copy items from [CURSOR-START-AMD-RDNA2-IGPU.md](../../../docs/CURSOR-START-AMD-RDNA2-IGPU.md) §10. Mark each **OPEN** / **RESOLVED** with cite.

| # | Question | Status | Cite / notes |
|---|---|---|---|
| 1 | Exact IOKit nub / ACPI path for Raphael iGPU | OPEN | iGPU **recognized** in Monterey System Information — dump IORegistry next to lock path/DID/rev |
| 2 | Tahoe X6000 IOGPU user-client identity vs older traces | OPEN | Need Tahoe + AMD dGPU oracle (5080 cannot provide X6000 ABI) |
| 3 | macOS redistrib license for Raphael firmware blobs | OPEN | |
| 4 | CompilerPluginInterface vs in-bundle AIR→gfx1036 | OPEN | |
| 5 | Honest minimum MTLGPUFamily for 2 CU UMA | OPEN | |
| 6 | X6000 DCN UC closeness to DCN 3.1.5 | OPEN | |
| 7 | Rembrandt DID table vs shared match code | OPEN | deferred until R7 |
| 8 | Primary display policy | RESOLVED | Connector-driven (§2.1). Boot-GOP handoff with dual cables still OPEN to measure. |
| 9 | Does stock WEG patch or touch DID `0x164E`? | OPEN | Measure with WEG DEBUG on R0 board |
| 10 | Nvidia/unsupported dGPU companion: attach isolation | **PARTIAL** | Monterey: iGPU unaccelerated boot **with RTX 5080 present** and both listed in Graphics. Confirms enumeration coexistence. Still need: our kext does not claim `10de:*`; Tahoe re-check. |

## Lab notes

- Unaccelerated Monterey desktop on iGPU ⇒ G2 is not starting from zero; we replace/own the FB path rather than invent first light.
- Product pin remains **Tahoe**; Monterey evidence informs match/FB risk only.
- Dual Metal device tests require an Apple-supported AMD dGPU (or second machine); 5080 alone is PCI coexistence only.

**Rule:** do not invent answers. Measure or cite; otherwise leave OPEN and stop that branch.
