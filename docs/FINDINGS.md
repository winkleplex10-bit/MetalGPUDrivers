# FINDINGS.md — living lab trace log

Append-only observations from hardware sessions. Cite OS build, command, and path to raw dumps.

## 2026-09-07 — Nvidia RTX 5080 Phase 0 / N1 baseline (Sequoia 15.7.9)

- Host: MacPro7,1 SMBIOS Hackintosh; dual GPU with Raphael `1002:164E`
- Nvidia: `10de:2c02` rev `A1`, subsystem `1462:5315`, Slot-1, BDF `1:0:0`
- Link: PCIe x16 @ 32 GT/s (`IOPCIExpressLinkStatus=0x1105`)
- BARs: 64 MiB @ `0xd8000000`, 16 B @ `0xf2000000`, 32 MiB @ `0xf0000000`, 512 KiB @ `0xdc000000`
- FB: Apple `IONDRVFramebuffer` attached; System Information **No Kext Loaded** for Nvidia
- Raphael: `RaphaelController` / `RaphaelAccelerator` present (`RaphaelPhase=R1-enumerate`)
- Built: `ihv/nvidia/build/NvidiaGSP.kext` (enumerate-only; not yet injected this session)
- Dumps: `ihv/nvidia/docs/traces/sequoia-rtx5080/`

## UNKNOWN raised

- macOS redistribution of GB20x GSP firmware (blocks N1.4)
