# AMD RDNA 3.5 iGPU — Strix Point 800M / Ryzen AI 300

UMA APU backend for Hackintosh-on-AMD-APU platforms. Display is the APU's DCN; not a Mac Pro dGPU slot.

**Binding spec:** [docs/CURSOR-START-AMD.md](../../docs/CURSOR-START-AMD.md) §4.4, §7 Phase 8

## Scope

- **First target:** Strix Point Radeon 890M (`gfx1150`)
- **Not:** Hawk Point 700M (RDNA 3, not 3.5) — separate backend if needed
- Matching is platform/ACPI-shaped, not a Thunderbolt dGPU personality

## Layout

```
firmware/     PSP 14 / GC 11.5 / DCN 3.5
match/        APU/platform match (not a 7000 DID list)
display/      APU DCN is the system display
submit/
isa/          AIR → gfx1150/1151/1152
uma/          Shared-memory / IOSurface rules
```

## Phase status

Not started. Begin after RDNA3 (+ optionally RDNA4) dGPU path is proven.
