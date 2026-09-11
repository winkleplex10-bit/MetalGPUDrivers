# Raphael PSP / GC / DCN firmware

Raphael (Ryzen 7000 APU) IP from the [kernel APU ASIC table](https://www.kernel.org/doc/Documentation/gpu/amdgpu/apu-asic-info-table.csv):

| IP | Version | linux-firmware names |
|---|---|---|
| DCN | 3.1.5 | `dcn_3_1_5_dmcub.bin` — **bundled unmodified** (build-time fetch; 0.2.6+) |
| MP0 / PSP | 13.0.5 | `psp_13_0_5_toc.bin`, `psp_13_0_5_ta.bin` — bundled (Linux DCN 3.1.5 loads DMCUB via PSP) |
| GC | 10.3.6 | `gc_10_3_6_*.bin` (pfp/me/mec/rlc) — not in this slice |
| VCN | 3.1.2 | `vcn_3_1_2.bin` — not in this slice |
| SDMA | 5.2.6 | `sdma_5_2_6.bin` — not in this slice |
| MP1 / SMU | 13.0.5 | `smu_13_0_5.bin` — not in this slice |

Blobs:

- https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/tree/amdgpu/dcn_3_1_5_dmcub.bin
- GitHub mirror: https://github.com/linux-firmware/linux-firmware/blob/main/amdgpu/dcn_3_1_5_dmcub.bin
- WHENCE: `Licence: Redistributable. See LICENSE.amdgpu for details.`

## License (LICENSE.amdgpu)

Same grant as LicenseDB [amd-linux-firmware](https://scancode-licensedb.aboutcode.org/amd-linux-firmware.html) / LICENSE.radeon wording:

- Binary install / copy / distribute only.
- **No reverse engineering, decompilation, or disassembly.**
- Must reproduce copyright + permission notice + disclaimers with redistributions.
- **No OS-restriction clause** — macOS kext redistribution of the unmodified blob is not blocked by the text.

Full text: [LICENSE.amdgpu](LICENSE.amdgpu) (also copied into `RaphaelIGPU.kext/Contents/Resources/LICENSE.amdgpu`). Mirror: https://raw.githubusercontent.com/thesofproject/linux-firmware/master/LICENSE.amdgpu

Do **not** commit `*.bin` / `*.fw`. Do **not** patch blobs. Do **not** copy GPL Linux driver C into the kext.

## Fetch and bundle

```
make -C ihv/amd-rdna2-igpu fetch-firmware
make -C ihv/amd-rdna2-igpu kext
```

`make kext` fetches if `firmware/cache/dcn_3_1_5_dmcub.bin` is missing, then copies blobs + LICENSE into `build/RaphaelIGPU.kext/Contents/Resources/`. Runtime load looks at `/Library/Extensions/RaphaelIGPU.kext/Contents/Resources/` (AuxKC install path).

Linux DCN 3.1.5 (`dcn315_resource.c` `dmcub_support=true`, `amdgpu_dm.c` `FIRMWARE_DCN_315_DMUB`) authenticates DMCUB through PSP (`AMDGPU_UCODE_ID_DMCUB`, `psp_v13_0.c` `psp_13_0_5_{toc,ta}`). GOP on this lab left DMCUB running (`ENABLE=1 mailbox_rdy=1`) with `dal_fw=0` (`SCRATCH0=0x42`). 0.2.6 treated that as a PSP abort; **0.2.7** GPINTs anyway (`GET_FW_VERSION` response `0x05000649`). Cold PSP C2PMSG is **not** in `dcn_3_1_5_offset.h` (BAR5 is DCN) — later slice if ENABLE or mailbox_rdy is off. Inbox `base=0x80000000` may be FB/GART; do not map BAR0 to chase it.

This kext’s **0.2.7** drop wraps GOP scanout by default. `raphael_dcn_modeset=1` loads the host copy of `dcn_3_1_5_dmcub.bin`, GPINTs GOP DMCUB even when `dal_fw=0`, then **reaffirms** the live OTG0 3840×2160 pipe (physical **DP** jack). That is not a new PLL. Next slice: changing modeset, still no firmware reverse.
