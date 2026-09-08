# challenges.md — Apple Silicon display track

Living attack log. Full table: [CURSOR-START-APPLE-SILICON.md](../../../docs/CURSOR-START-APPLE-SILICON.md) §3.

| ID | Challenge | Status | Notes |
|---|---|---|---|
| C1 | DCP owns lid | Open | Prefer external heads for AS2 |
| C2 | AGX system Metal device | Open | Second MTLDevice; no AGX spoof |
| C3 | TinyGPU is compute-only | Known | Not a substitute for AS2/AS6 |
| C4 | No Graphics DriverKit | Open | Kext / documented surfaces |
| C5 | ADT / tunnel match | Open | Measure on frozen board |
| C6 | TB yank / 0xFFFFFFFF | Open | |
| C7 | Cross-device IOSurface | UNKNOWN | |
| C8 | Policy 102363 | Known | Does not cancel track |
| C9 | Signed load on AS | Out of recipe scope | Dumps on failure |
| C10 | AIR → vendor ISA | Open | Shared with x86 IHVs |

**Policy:** challenges are tracked here; they do **not** block opening the track or running AS0.
