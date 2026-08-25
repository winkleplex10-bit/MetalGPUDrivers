//
// Unaccelerated NVIDIA IOFramebuffer.
//
// Claims IOPCIDevice VGA/3D functions (vendor 0x10DE) with a probe score
// above leftover Kepler NVDAStartup, so Resman/GeForce never attach and
// cannot hang boot. If UEFI GOP's framebuffer lives in this card's BAR,
// WindowServer gets a single-mode linear aperture (installer + desktop,
// no acceleration, no mode changes).
//

#ifndef NV_METAL20_GOP_FRAMEBUFFER_HPP
#define NV_METAL20_GOP_FRAMEBUFFER_HPP

#include <IOKit/IOLib.h>
#include <IOKit/IODeviceMemory.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/graphics/IOFramebuffer.h>

class NvidiaGopFramebuffer20 : public IOFramebuffer
{
    OSDeclareDefaultStructors(NvidiaGopFramebuffer20);
    using super = IOFramebuffer;

public:
    bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
    void free() APPLE_KEXT_OVERRIDE;

    IOReturn enableController() APPLE_KEXT_OVERRIDE;
    bool isConsoleDevice() APPLE_KEXT_OVERRIDE;

    IODeviceMemory *getApertureRange(IOPixelAperture aperture) APPLE_KEXT_OVERRIDE;
    IODeviceMemory *getVRAMRange() APPLE_KEXT_OVERRIDE;

    const char *getPixelFormats() APPLE_KEXT_OVERRIDE;
    IOItemCount getDisplayModeCount() APPLE_KEXT_OVERRIDE;
    IOReturn getDisplayModes(IODisplayModeID *allDisplayModes) APPLE_KEXT_OVERRIDE;
    IOReturn getInformationForDisplayMode(IODisplayModeID displayMode,
                                          IODisplayModeInformation *info) APPLE_KEXT_OVERRIDE;
    UInt64 getPixelFormatsForDisplayMode(IODisplayModeID displayMode,
                                         IOIndex depth) APPLE_KEXT_OVERRIDE;
    IOReturn getPixelInformation(IODisplayModeID displayMode, IOIndex depth,
                                 IOPixelAperture aperture,
                                 IOPixelInformation *pixelInfo) APPLE_KEXT_OVERRIDE;
    IOReturn getCurrentDisplayMode(IODisplayModeID *displayMode,
                                   IOIndex *depth) APPLE_KEXT_OVERRIDE;
    IOReturn setDisplayMode(IODisplayModeID displayMode, IOIndex depth) APPLE_KEXT_OVERRIDE;
    IOReturn getStartupDisplayMode(IODisplayModeID *displayMode,
                                   IOIndex *depth) APPLE_KEXT_OVERRIDE;
    IOReturn getTimingInfoForDisplayMode(IODisplayModeID displayMode,
                                         IOTimingInformation *info) APPLE_KEXT_OVERRIDE;

    IOItemCount getConnectionCount() APPLE_KEXT_OVERRIDE;
    IOReturn getAttribute(IOSelect attribute, uintptr_t *value) APPLE_KEXT_OVERRIDE;
    IOReturn setAttribute(IOSelect attribute, uintptr_t value) APPLE_KEXT_OVERRIDE;
    IOReturn getAttributeForConnection(IOIndex connectIndex, IOSelect attribute,
                                       uintptr_t *value) APPLE_KEXT_OVERRIDE;
    IOReturn connectFlags(IOIndex connectIndex, IODisplayModeID displayMode,
                          IOOptionBits *flags) APPLE_KEXT_OVERRIDE;

private:
    static constexpr IODisplayModeID kGopModeID = 1;

    IOPCIDevice *fPCI = nullptr;
    bool fHaveGop = false;
    bool fClaimOnly = false;
    bool fIsConsole = false;
    UInt16 fVendor = 0;
    UInt16 fDevice = 0;

    IOPhysicalAddress fFbPhys = 0;
    IOPhysicalLength fFbLength = 0;
    UInt32 fWidth = 0;
    UInt32 fHeight = 0;
    UInt32 fRowBytes = 0;
    UInt32 fDepth = 32;
    SInt32 fVramBarIndex = -1;

    bool parseBootVideo();
    bool gopLivesInThisDevice();
    void publishIdentity();
    void fillPixelInfo(IOPixelInformation *pixelInfo) const;
};

#endif
