# host/iofb — linear IOFramebuffer shell

Vendor-agnostic `IOFramebuffer` subclass used by IHV backends that need a GOP/linear wrap before real modeset.

`IOFBLinearShell` implements the usual IOGraphics pure virtuals (modes, 32-bit XRGB, software cursor, timer VBL). Subclasses supply `copyApertureMemory()`.
