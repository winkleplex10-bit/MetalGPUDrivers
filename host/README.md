# Host — vendor-agnostic macOS GPU integration

The host layer speaks Apple's frameworks and IOKit families. IHV backends (`ihv/*`) plug in behind stable contracts defined in `CURSOR-IHV-DRIVER-SPEC.md` (pending).

## Directories

| Path | Role |
|---|---|
| `iofb/` | `IOFBLinearShell` — linear `IOFramebuffer` (used by Raphael GOP wrap) |
| `ioaccel/` | Placeholder — no `IOAccelDevice` until IOGPU selectors are traced |
| `air/` | Shared AIR ingest helpers (`.metallib` slices, triple validation) |
| `mtl-plugin/` | Notes; IHV supplies `*MTLDriver.bundle` (Raphael stub is off by default) |
| `present/` | IOSurface-backed present path to WindowServer |
| `power/` | Power-helper shape (clocks, thermal, surprise-remove) — no AGPM injector |

## Contract

- `host/` must compile against a **stub IHV** until a vendor backend is linked
- Adding a new IHV directory must **not** require edits to another vendor's ISA or firmware trees
- Bundle IDs are project-owned, never `com.apple.*` or historical vendor web-driver IDs

## Reference

Host-wide phases, acceptance tests, and open questions: `CURSOR-IHV-DRIVER-SPEC.md` (to be added).
