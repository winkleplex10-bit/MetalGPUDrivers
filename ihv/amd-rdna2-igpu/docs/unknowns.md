# unknowns.md — blockers for `ihv/amd-rdna2-igpu`

| # | Question | Status | Cite / notes |
|---|---|---|---|
| 1 | Exact IOKit nub / ACPI path for Raphael iGPU | **RESOLVED** | `IGPU@0`, ACPI `_SB.PCI0.GP17.VGA`, PCI `12:0:0`, DID `164E` rev `C9`, subsys `1043:8877`. Trace: `docs/traces/sequoia-7950x3d/ioreg-igpu-excerpt.txt` |
| 2 | Tahoe/Sequoia X6000 IOGPU user-client identity | OPEN | Need AMD dGPU oracle on same OS major as bring-up |
| 3 | macOS redistrib license for Raphael firmware blobs | OPEN | |
| 4 | CompilerPluginInterface vs in-bundle AIR→gfx1036 | OPEN | |
| 5 | Honest minimum MTLGPUFamily for 2 CU UMA | OPEN | |
| 6 | X6000 DCN UC closeness to DCN 3.1.5 | OPEN | |
| 7 | Rembrandt DID table vs shared match code | OPEN | deferred until R7 |
| 8 | Primary display policy | **RESOLVED** | Connector-driven. Lab currently APU-main (4K). |
| 9 | Does stock/fork WEG patch DID `0x164E`? | OPEN | WEG **1.7.1d7 laobamac** is loaded; no WEG property obvious on iGPU node in excerpt — confirm with WEG DEBUG |
| 10 | Nvidia companion attach isolation | **PARTIAL → mostly RESOLVED for enum** | Sequoia: iGPU main display + RTX 5080 `GFX0` present, no Metal on either. Still need: our kext never matches `10de:2c02`. |
| 11 | Interaction with Apple `AMDSupport` on Raphael | OPEN | AMDSupport already attaches (vendor-wide AMD VGA). Decide: coexist as sibling vs claim FB match category and supersede NDRV. Measure probe scores before coding G2. |
| 12 | Physical motherboard port for 4K main display | OPEN | User: which HDMI/DP jack? |
| 13 | Bring-up OS: Sequoia vs Tahoe product pin | OPEN | Dump is **15.7.8**. Product docs say Tahoe. Prefer develop on Sequoia now; re-validate on Tahoe before ship. |

## Lab notes

- CPU is **7950X3D** (not 7950X3D typo in older drafts — confirmed System Information).
- G2 is not greenfield: `.display_boot` / `IONDRVFramebuffer` already owns a 4K head (`IOFBMemorySize` ≈ 33 177 600 ≈ 3840×2160×4).
- `gfxutil -f display` produced an empty file — optional retry later.

**Rule:** do not invent answers. Measure or cite; otherwise leave OPEN and stop that branch.
