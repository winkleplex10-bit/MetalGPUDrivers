# RaphaelMTLDriver.bundle (stub)

`MetalPluginName` / `MetalPluginClassName` for a future AIR → `gfx1036` plugin.

**Do not enable** `raphael_metal=1` until this bundle actually implements the Metal IHV SPI. Advertising the name with no compiler makes Metal.framework fail on Sequoia.

The kext does not embed or register this plugin by default.

Search path for `MetalPluginName` on Sequoia (bundle next to the kext vs `/System/Library/Extensions`) is **UNKNOWN** until traced from a live `AMDRadeonX6000` stack.

| Field | Value |
|---|---|
| Bundle name | `RaphaelMTLDriver` |
| Class | `RaphaelMTLDriver` |
| Bundle ID | `dev.metalgpudrivers.RaphaelMTLDriver` |
