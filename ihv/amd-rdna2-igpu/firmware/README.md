# Raphael PSP / GC / DCN firmware

Raphael (Ryzen 7000 APU) IP from the [kernel APU ASIC table](https://www.kernel.org/doc/Documentation/gpu/amdgpu/apu-asic-info-table.csv):

| IP | Version | Typical linux-firmware names (study only) |
|---|---|---|
| DCN | 3.1.5 | `dcn_3_1_5_dmcub.bin` |
| GC | 10.3.6 | `gc_10_3_6_*.bin` (pfp/me/mec/rlc) |
| VCN | 3.1.2 | `vcn_3_1_2.bin` |
| SDMA | 5.2.6 | `sdma_5_2_6.bin` |
| MP0 / PSP | 13.0.5 | `psp_13_0_5_{sos,ta}.bin` |
| MP1 / SMU | 13.0.5 | `smu_13_0_5.bin` |

## License

**UNKNOWN** whether these blobs may be redistributed inside a macOS kext. linux-firmware terms are not a macOS grant.

Do not commit `*.bin` / `*.fw` here. Phase R1 firmware heartbeat stops until a redistrib license is documented.

This kext’s first drop wraps GOP scanout and does not load PSP/GC/DCN firmware.
