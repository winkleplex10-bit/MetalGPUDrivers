# Clocks and power — Nvidia

- All clocks are **GSP-brokered** via open-rm RPC paths
- No unsigned Falcon firmware plans
- No AGPM injector or WhateverGreen-style spoofing
- Maxwell2+ signed firmware requires GSP; do not plan pre-GSP reclocking

See CURSOR-START-NVIDIA §5 (signed firmware / clocks) and §8 do-nots.
