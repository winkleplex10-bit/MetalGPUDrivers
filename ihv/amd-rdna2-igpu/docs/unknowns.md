# unknowns.md — blockers for `ihv/amd-rdna2-igpu`

Copy items from [CURSOR-START-AMD-RDNA2-IGPU.md](../../../docs/CURSOR-START-AMD-RDNA2-IGPU.md) §10. Mark each **OPEN** / **RESOLVED** with cite.

| # | Question | Status | Cite / notes |
|---|---|---|---|
| 1 | Exact IOKit nub / ACPI path for Raphael iGPU | OPEN | |
| 2 | Tahoe X6000 IOGPU user-client identity vs older traces | OPEN | |
| 3 | macOS redistrib license for Raphael firmware blobs | OPEN | |
| 4 | CompilerPluginInterface vs in-bundle AIR→gfx1036 | OPEN | |
| 5 | Honest minimum MTLGPUFamily for 2 CU UMA | OPEN | |
| 6 | X6000 DCN UC closeness to DCN 3.1.5 | OPEN | |
| 7 | Rembrandt DID table vs shared match code | OPEN | deferred until R7 |
| 8 | Primary display policy | RESOLVED | Connector-driven (§2.1). Boot-GOP handoff with dual cables still OPEN to measure. |
| 9 | Does stock WEG patch or touch DID `0x164E`? | OPEN | Measure with WEG DEBUG on R0 board |
| 10 | Nvidia/unsupported dGPU companion: attach isolation | OPEN | Must not break iGPU attach |

**Rule:** do not invent answers. Measure or cite; otherwise leave OPEN and stop that branch.
