# Intel Arc IHV slot — discrete Xe-HPG / Xe2

Intel Arc dGPU backend. Apple never shipped Arc as a Metal device.

**Binding spec:** [docs/CURSOR-START-INTEL.md](../../docs/CURSOR-START-INTEL.md)

## Scope

- **Phase 0 default:** One Alchemist board (A750/A770), VID `0x8086`
- **Later:** Battlemage B-series (Xe2-HPG) — different GuC/HuC pair, same backend shape
- **Out:** Ponte Vecchio, DG1 Iris Xe MAX, spoofing `AppleIntel*` personalities

## Layout

```
firmware/     GuC/HuC/GSC/DMC (blobs not in git unless licensed)
pci/          8086 + A-series / B-series DID tables
display/      Arc DE, CDCLK, card connectors, VRAM scanout
submit/       GuC CTB / compute classes
isa/          AIR → Xe (Alchemist / Battlemage variants)
```

## Phase status

Not started. Host Phase 7 slot — after Nvidia or in parallel once `host/` is stable.
