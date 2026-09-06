# host/ioaccel

Placeholder for a future `IOAcceleratorFamily2` / `IOGPUFamily` user-client shell.

Raphael’s first kext uses a plain `IOService` (`RaphaelAccelerator`) and does **not** subclass `IOAccelDevice`, so WindowServer will not open IOGPU selectors we have not traced.
