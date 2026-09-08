# Apple Silicon IHV track — display acceleration

ARM / Apple Silicon platform integration for **accelerated display** (not compute-only).

**Binding specs:**
- [docs/BUILD-RULES.md](../../docs/BUILD-RULES.md) — macOS 26 pin, WhateverGreen absent, AS challenges
- [docs/CURSOR-START-APPLE-SILICON.md](../../docs/CURSOR-START-APPLE-SILICON.md) — phased plan

## Intent

Enumerate a foreign/sidecar GPU on an M-series Mac, bring firmware up, drive **external** heads on the card when possible, then Metal present — with AGX/DCP challenges documented and attacked, **not** used as a project stop.

## Layout

```
docs/       boards, challenges log, unknowns
match/      ADT / tunnelled PCIe match
display/    External scanout first; DCP notes later
submit/     AS glue to vendor submit paths
metal/      *MTLDriver.bundle for AS
firmware/   License notes only until grant
```

## Phase status

Not started (scaffold only). Target OS: **macOS 26**. WhateverGreen: **absent**.
