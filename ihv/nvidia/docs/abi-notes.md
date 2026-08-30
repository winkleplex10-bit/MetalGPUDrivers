# Nvidia ABI notes

Observed RPC class IDs, open-gpu-doc headers opened, and Tahoe IORegistry traces.

**Status:** Empty — populate during Phase 0–1 bring-up. Cite filenames and build numbers; do not guess selectors or RPC IDs.

## Open questions (from CURSOR-START-NVIDIA §9)

1. macOS GSP firmware redistribution
2. GSP graphics vs compute feature set on macOS
3. IOGPU vs IOAccelContext2 on Tahoe Intel
4. CompilerPluginInterface vs in-bundle compilation
5. Exact Ampere compute class + QMD for frozen DID
6. Display-engine class for GA10x
7. Minimum honest MTLGPUFamily for Ampere
8. Cross-device IOSurface without CPU
9. AGDC / GPUWrangler for internal PCIe FB
10. Blackwell surface-kind / QMD deltas
