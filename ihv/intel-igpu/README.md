# Intel modern iGPU — Xe-LPG / Xe2-LPG

Backend for Intel iGPUs Apple never shipped (Meteor Lake Xe-LPG, Lunar Lake Xe2-LPG, etc.).

**Binding spec:** [docs/CURSOR-START-INTEL.md](../../docs/CURSOR-START-INTEL.md) §3.2

## Scope

- UMA memory model; package display engine is the boot display on Hackintosh
- PCI function typically `00:02.0` (IGD)
- **Not:** Gen9/11 AppleIntel KBL/ICL stack — trace reference only
- **Not:** Apple Silicon AGX

## Layout

```
firmware/     MTL/ARL/LNL GuC/HuC/DMC — not Arc blobs
pci/          IGD DID tables
display/      Package DE, GOP handoff, UMA FB
submit/
isa/          AIR → Xe-LPG / Xe2 (not Gen9 EU)
```

## Phase status

Not started. Pick Arc **or** iGPU for first Intel board in Phase 0 — do not dual-bring-up.
