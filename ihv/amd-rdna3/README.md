# AMD RDNA3 IHV slot — RX 7000 / Navi 3x

**First AMD implementation target.** Unofficial display + Metal for GPUs Apple never shipped.

**Binding spec:** [docs/CURSOR-START-AMD.md](../../docs/CURSOR-START-AMD.md)

## Scope

- **Preferred first board:** Navi 33 / `gfx1102` / RX 7600 (monolithic, simpler than 7900 chiplet)
- **ABI oracle:** Live `AMDRadeonX6000` stack on Tahoe Intel macOS — trace, do not spoof
- **Out:** RX 6000 personality spoof onto 7000; Hawk Point mislabeled as 800M

## Layout

```
firmware/     PSP 13 / SMU 13 / GC 11.0 (blobs not in git unless licensed)
pci/          Navi 3x VID/DID; IOPCITunnelCompatible if TB
display/      DCN 3.2.x + Radiance
submit/       MES / PM4 / compute classes
isa/          AIR → gfx1100/1101/1102
chiplet/      MCD mask, harvest, GCD↔MCD map (7900 SKU hardening)
```

## Phase status

Not started. See CURSOR-START-AMD §7 for P0–P6 acceptance criteria.
