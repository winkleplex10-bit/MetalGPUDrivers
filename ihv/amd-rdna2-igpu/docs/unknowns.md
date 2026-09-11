# unknowns.md — blockers for `ihv/amd-rdna2-igpu`

| # | Question | Status | Cite / notes |
|---|---|---|---|
| 1 | Exact IOKit nub / ACPI path for Raphael iGPU | **RESOLVED** | Sequoia: `IGPU@0`. **Tahoe:** `VGA@0` under `GP17@8,1`, `pci1002,164e`. ACPI `_SB.PCI0.GP17.VGA`, PCI `12:0:0`, DID `164E` rev `C9`, subsys `1043:8877`. DID-only match; nub rename is not a blocker. Sequoia trace: `docs/traces/sequoia-7950x3d/ioreg-igpu-excerpt.txt` |
| 2 | Tahoe X6000 IOGPU user-client identity | OPEN | Need AMD dGPU oracle on **Tahoe** (bring-up OS) |
| 3 | macOS redistrib license for Raphael firmware blobs | **RESOLVED for DCN/PSP display blobs** | linux-firmware `LICENSE.amdgpu`: binary install/copy/distribute; no reverse/decompile/disassemble; no OS-restriction. LicenseDB [amd-linux-firmware](https://scancode-licensedb.aboutcode.org/amd-linux-firmware.html). 0.2.6+ fetches unmodified `dcn_3_1_5_dmcub.bin` + `psp_13_0_5_{toc,ta}.bin`. GC/SMU/VCN still unused. |
| 4 | CompilerPluginInterface vs in-bundle AIR→gfx1036 | OPEN | |
| 5 | Honest minimum MTLGPUFamily for 2 CU UMA | OPEN | |
| 6 | X6000 DCN UC closeness to DCN 3.1.5 | OPEN | |
| 7 | Rembrandt DID table vs shared match code | OPEN | deferred until R7 |
| 8 | Primary display policy | **RESOLVED** | Connector-driven. Lab currently APU-main (4K). |
| 9 | Does WEG patch DID `0x164E`? | **N/A on current lab** | Tahoe bring-up has **no WEG**. Sequoia dump had WEG 1.7.1d7 laobamac; unanswered there. Do not depend on WEG either way. |
| 10 | Nvidia companion attach isolation | **RESOLVED on box** | DID-only `0x164E1002`. Tahoe: 5080 `IONDRVFramebuffer` untouched; iGPU wrap did not claim `GPP0`. |
| 11 | Interaction with Apple `AMDSupport` on Raphael | **RESOLVED on box** | AMDSupport stays `IOMatchCategory=AMDSupport`. Controller `RaphaelHW`. FB `IOFramebuffer` probe **100000** vs NDRV **20000**. Tahoe: AMDSupport sibling + our FB on `VGA@0`. |
| 12 | Physical motherboard port for 4K main display | **RESOLVED** | User: cable is **DisplayPort**, not HDMI. Live pipe OTG0 3840×2160, HPD2 SENSE. VFCT path 0 HDMI-A `0x320C`; path 2 DP `0x3113` — ATOM names disagree with silkscreen. Boot head **DP index 1**. |
| 13 | Bring-up OS | **LOCKED Tahoe** | **macOS 26 Tahoe, no WEG.** IORegistry `OS Build Version` **25G83**, Darwin 25.6.0. Sequoia 15.7.8 dump is historical only. |
| 14 | Patch Raphael into Apple X6000 as Navi 2 | **REJECTED** | Match-level spoof ≠ DCN 3.1.5 / UMA / gfx1036. `AMDSupport` on the iGPU is not Metal. See plan §0.2 |
| 15 | DCN 3.1.5 hardware modeset / DMUB | **GPINT + reaffirm on-box 0.2.7** | `dcn315_resource.c` `dmcub_support=true`. 0.2.6: ENABLE=1 mailbox_rdy=1 dal_fw=0 (SCRATCH0=0x42) aborted before GPINT. **0.2.7** GPINTs on ENABLE+mailbox_rdy (`GET_FW_VERSION=0x05000649`); OTG0 reaffirm same GOP totals. Inbox `base=0x80000000` may be FB/GART — BAR0 map still forbidden. **Next:** changing modeset (blank/unblank, new timing, or live DP DIG/PHY), GOP fallback, still `raphael_dcn_modeset=1`. Cold PSP MP0 only if GOP DMCUB dies. Dual HDMI+DP after single-head can change. Do not re-gate on `dal_fw`. |

## Lab notes

- CPU is **7950X3D**. Tahoe build **25G83**.
- R1/R2 **on-box (0.2.2):** `RaphaelController` + `RaphaelFramebuffer` on `VGA@0`. WindowServer `fb0=/RaphaelFramebuffer`. iGPU `.Display_boot` gone. 5080 still has `IONDRVFramebuffer`.
- GOP wrap geometry matches the old NDRV 4K size (`IOFBMemorySize` 33177600). Console `v_baseAddr=0x10000000001` is not a usable physical (blue tint on 0.2.2). v0.2.3 takes the aperture from a PCI BAR (`phys=0x10000000000` pitch 15360). v0.2.4: HDMI+DP fallback (VFCT), `kConnectionFlags` 0 on external, BAR5 probe **on-box**. v0.2.5: HPD named bits + OTG dump. v0.2.6: boot jack **DP**; aborted GPINT on `dal_fw=0`. **v0.2.7:** GPINT + OTG0 4K reaffirm; AuxKC `/Library/Extensions`; boot-args `-v keepsyms=1 debug=0x100 raphael_dcn_modeset=1`.
- `gfxutil -f display` produced an empty file on Sequoia — optional retry later.

**Rule:** do not invent answers. Measure or cite; otherwise leave OPEN and stop that branch.
