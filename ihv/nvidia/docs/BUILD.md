# NvidiaGSP — Phase N1 enumerate kext (Blackwell RTX 5080)

## What it does

- Matches **exactly** `IOPCIPrimaryMatch = 0x2c0210de` (RTX 5080)
- Attaches under `IOMatchCategory = NvidiaHW` (does **not** steal `IOFramebuffer` from `IONDRVFramebuffer`)
- Enables PCI memory space, retains BAR `IODeviceMemory` descriptors, logs sizes
- Publishes `NvidiaPhase=N1-enumerate`, `NvidiaGsp=false`, `NvidiaMetal=false`

## What it does **not** do

- Boot GSP / load firmware blobs
- Claim `IOFramebuffer` / modeset
- Enable bus-master DMA
- Touch HDMI audio function `10de:22e9`
- Invent RPC IDs or MMIO register pokes

## Darwin build

```bash
cd ihv/nvidia
make kext
# or:
xcodebuild -project NvidiaGSP.xcodeproj -scheme NvidiaGSP -configuration Release build
```

Output:
- `make`: `ihv/nvidia/build/NvidiaGSP.kext`
- Xcode: `ihv/nvidia/build/xcode/Release/NvidiaGSP.kext`

Open `NvidiaGSP.xcodeproj` in Xcode for editing/building. Product type is a kernel extension (`wrapper.kext`).

## Load (lab)


Assumes OpenCore / AuxKC loading is already solved on this host (same path as `RaphaelIGPU.kext`).

- Inject `NvidiaGSP.kext` after Lilu if present
- Boot-args:
  - `-nvoff` — do not attach
  - `nv_map=0` — claim without retaining BAR descriptors

## Verify

```bash
log show --last boot --predicate 'eventMessage CONTAINS "NvidiaController" OR eventMessage CONTAINS "NvidiaGSP"'
ioreg -l -w0 | grep -A30 NvidiaController
```

Expect `NvidiaController` under `GFX0@0` alongside existing `IONDRVFramebuffer`.
