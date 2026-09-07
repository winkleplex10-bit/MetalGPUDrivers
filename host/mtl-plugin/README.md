# host/mtl-plugin

Vendor-agnostic notes for `*MTLDriver.bundle` discovery via `MetalPluginName`.

Raphael’s stub lives in [`ihv/amd-rdna2-igpu/metal/`](../../ihv/amd-rdna2-igpu/metal/). It is **not** registered unless the IHV kext sets `MetalPluginName` (`raphael_metal=1`, default off).
