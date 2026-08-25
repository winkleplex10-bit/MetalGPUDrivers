//
// NvidiaGopFramebuffer30.cpp
// Unaccelerated NVIDIA GOP framebuffer for macOS / Hackintosh.
//

#include "NvidiaGopFramebuffer.hpp"
#include "NvidiaPciIds.h"

#include <libkern/OSByteOrder.h>
#include <libkern/libkern.h>
#include <pexpert/pexpert.h>
#include <pexpert/i386/boot.h>

#define NVFB_LOG(fmt, ...) \
    IOLog("NvidiaMetal30: " fmt "\n", ##__VA_ARGS__)

OSDefineMetaClassAndStructors(NvidiaGopFramebuffer30, IOFramebuffer);

static bool bootArgPresent(const char *name)
{
    const char *args = PE_boot_args();
    if (args == nullptr || name == nullptr) {
        return false;
    }
    const size_t n = strlen(name);
    for (const char *p = args; *p != '\0'; p++) {
        if (strncmp(p, name, n) != 0) {
            continue;
        }
        if (p != args && p[-1] != ' ') {
            continue;
        }
        const char after = p[n];
        if (after == '\0' || after == ' ' || after == '=') {
            return true;
        }
    }
    return false;
}

bool NvidiaGopFramebuffer30::parseBootVideo()
{
    boot_args *args = static_cast<boot_args *>(PE_state.bootArgs);
    PE_Video console = {};

    if (args != nullptr && args->Video.v_baseAddr != 0 &&
        args->Video.v_width != 0 && args->Video.v_height != 0) {
        fFbPhys   = static_cast<IOPhysicalAddress>(args->Video.v_baseAddr);
        fWidth    = args->Video.v_width;
        fHeight   = args->Video.v_height;
        fRowBytes = args->Video.v_rowBytes;
        fDepth    = args->Video.v_depth;
    } else if (args != nullptr && args->VideoV1.v_baseAddr != 0) {
        fFbPhys   = args->VideoV1.v_baseAddr;
        fWidth    = args->VideoV1.v_width;
        fHeight   = args->VideoV1.v_height;
        fRowBytes = args->VideoV1.v_rowBytes;
        fDepth    = args->VideoV1.v_depth;
    } else if (PE_current_console(&console) == 0 && console.v_width != 0) {
        // v_baseAddr here is a kernel virtual mapping; physical comes from boot_args.
        fWidth    = static_cast<UInt32>(console.v_width);
        fHeight   = static_cast<UInt32>(console.v_height);
        fRowBytes = static_cast<UInt32>(console.v_rowBytes);
        fDepth    = static_cast<UInt32>(console.v_depth);
        if (args != nullptr && args->Video.v_baseAddr != 0) {
            fFbPhys = static_cast<IOPhysicalAddress>(args->Video.v_baseAddr);
        }
    }

    if (fWidth == 0 || fHeight == 0) {
        return false;
    }

    if (fDepth != 16 && fDepth != 32) {
        fDepth = 32;
    }
    if (fRowBytes < fWidth * (fDepth / 8)) {
        fRowBytes = fWidth * (fDepth / 8);
    }

    fFbLength = static_cast<IOPhysicalLength>(fRowBytes) * fHeight;
    if (fFbLength == 0) {
        return false;
    }

    NVFB_LOG("GOP %ux%u depth=%u pitch=%u phys=0x%llx len=0x%llx",
             fWidth, fHeight, fDepth, fRowBytes,
             static_cast<uint64_t>(fFbPhys),
             static_cast<uint64_t>(fFbLength));
    return fFbPhys != 0;
}

bool NvidiaGopFramebuffer30::gopLivesInThisDevice()
{
    if (fPCI == nullptr || fFbPhys == 0 || fFbLength == 0) {
        return false;
    }

    const IOItemCount bars = fPCI->getDeviceMemoryCount();
    for (IOItemCount i = 0; i < bars; i++) {
        IODeviceMemory *mem = fPCI->getDeviceMemoryWithIndex(i);
        if (mem == nullptr) {
            continue;
        }
        const IOPhysicalAddress start = mem->getPhysicalAddress();
        const IOByteCount len = mem->getLength();
        if (len < fFbLength) {
            continue;
        }
        const uint64_t fb = static_cast<uint64_t>(fFbPhys);
        const uint64_t b0 = static_cast<uint64_t>(start);
        const uint64_t b1 = b0 + static_cast<uint64_t>(len);
        if (fb >= b0 && (fb + static_cast<uint64_t>(fFbLength)) <= b1) {
            fVramBarIndex = static_cast<SInt32>(i);
            NVFB_LOG("GOP is inside BAR index %u (0x%llx+0x%llx)",
                     i, b0, static_cast<uint64_t>(len));
            return true;
        }
    }
    return false;
}

void NvidiaGopFramebuffer30::publishIdentity()
{
    if (fPCI == nullptr) {
        return;
    }

    char model[96];
    snprintf(model, sizeof(model), "%s (unaccelerated)", NvidiaChipName(fDevice));
    fPCI->setProperty("model", model);
    fPCI->setProperty("NVDAType", "NvidiaMetal30");
    setProperty("model", model);

    if (fIsConsole) {
        fPCI->setProperty("AAPL,boot-display", kOSBooleanTrue);
        setProperty("AAPL,boot-display", kOSBooleanTrue);
    }
}

void NvidiaGopFramebuffer30::fillPixelInfo(IOPixelInformation *pixelInfo) const
{
    bzero(pixelInfo, sizeof(*pixelInfo));
    pixelInfo->bytesPerRow      = fRowBytes;
    pixelInfo->bitsPerPixel     = fDepth;
    pixelInfo->pixelType        = kIORGBDirectPixels;
    pixelInfo->componentCount   = 3;
    pixelInfo->bitsPerComponent = (fDepth == 16) ? 5 : 8;
    if (fDepth == 16) {
        pixelInfo->componentMasks[0] = 0x7C00;
        pixelInfo->componentMasks[1] = 0x03E0;
        pixelInfo->componentMasks[2] = 0x001F;
        strlcpy(pixelInfo->pixelFormat, IO16BitDirectPixels,
                sizeof(pixelInfo->pixelFormat));
    } else {
        pixelInfo->componentMasks[0] = 0x00FF0000;
        pixelInfo->componentMasks[1] = 0x0000FF00;
        pixelInfo->componentMasks[2] = 0x000000FF;
        strlcpy(pixelInfo->pixelFormat, IO32BitDirectPixels,
                sizeof(pixelInfo->pixelFormat));
    }
    pixelInfo->activeWidth  = fWidth;
    pixelInfo->activeHeight = fHeight;
}

bool NvidiaGopFramebuffer30::start(IOService *provider)
{
    if (bootArgPresent("-nvfboff")) {
        NVFB_LOG("disabled by -nvfboff");
        return false;
    }

    fPCI = OSDynamicCast(IOPCIDevice, provider);
    if (fPCI == nullptr) {
        return false;
    }

    fVendor = fPCI->configRead16(kIOPCIConfigVendorID);
    fDevice = fPCI->configRead16(kIOPCIConfigDeviceID);
    const UInt32 classReg = fPCI->configRead32(kIOPCIConfigRevisionID);
    const UInt8  baseClass = static_cast<UInt8>((classReg >> 24) & 0xFF);

    if (fVendor != 0x10DE || baseClass != 0x03) {
        NVFB_LOG("ignoring non-display NVIDIA function %04x:%04x class=%06x",
                 fVendor, fDevice, (classReg >> 8) & 0xFFFFFF);
        return false;
    }

    // Memory space only. Never map BAR0 or talk to GSP.
    fPCI->setMemoryEnable(true);

    fClaimOnly = bootArgPresent("-nvfbclaim");
    const bool forceGop = bootArgPresent("-nvfbforce");
    const bool haveVideo = parseBootVideo();

    if (haveVideo && gopLivesInThisDevice()) {
        fHaveGop = true;
        fIsConsole = true;
    } else if (haveVideo && forceGop) {
        // OpenCore DirectGopRendering can place the console in sysmem.
        fHaveGop = true;
        fIsConsole = true;
        NVFB_LOG("using GOP outside BAR because of -nvfbforce");
    } else {
        fHaveGop = false;
        fIsConsole = false;
        NVFB_LOG("claiming %s %04x:%04x without GOP (blocks Kepler NVDAStartup)",
                 NvidiaChipName(fDevice), fVendor, fDevice);
    }

    if (fClaimOnly) {
        fHaveGop = false;
        fIsConsole = false;
        NVFB_LOG("claim-only mode (-nvfbclaim)");
    }

    if (!super::start(provider)) {
        NVFB_LOG("IOFramebuffer::start failed");
        return false;
    }

    publishIdentity();
    NVFB_LOG("attached %s [%04x:%04x] console=%d gop=%d",
             NvidiaChipName(fDevice), fVendor, fDevice,
             fIsConsole ? 1 : 0, fHaveGop ? 1 : 0);
    return true;
}

void NvidiaGopFramebuffer30::stop(IOService *provider)
{
    fPCI = nullptr;
    fHaveGop = false;
    fIsConsole = false;
    fVramBarIndex = -1;
    super::stop(provider);
}

void NvidiaGopFramebuffer30::free()
{
    super::free();
}

IOReturn NvidiaGopFramebuffer30::enableController()
{
    if (!fHaveGop) {
        // Stay attached so NVDAStartup cannot match, but do not publish a
        // desktop. Typical when the boot display is an iGPU.
        NVFB_LOG("enableController: no GOP on this device, not a display");
        return kIOReturnUnsupported;
    }
    NVFB_LOG("enableController: GOP %ux%u", fWidth, fHeight);
    return kIOReturnSuccess;
}

bool NvidiaGopFramebuffer30::isConsoleDevice()
{
    return fIsConsole;
}

IODeviceMemory *NvidiaGopFramebuffer30::getApertureRange(IOPixelAperture aperture)
{
    if (aperture != kIOFBSystemAperture || !fHaveGop || fFbPhys == 0) {
        return nullptr;
    }
    return IODeviceMemory::withRange(fFbPhys, fFbLength);
}

IODeviceMemory *NvidiaGopFramebuffer30::getVRAMRange()
{
    if (fPCI != nullptr && fVramBarIndex >= 0) {
        IODeviceMemory *bar = fPCI->getDeviceMemoryWithIndex(
            static_cast<UInt32>(fVramBarIndex));
        if (bar != nullptr) {
            bar->retain();
            return bar;
        }
    }
    return getApertureRange(kIOFBSystemAperture);
}

const char *NvidiaGopFramebuffer30::getPixelFormats()
{
    return (fDepth == 16) ? IO16BitDirectPixels : IO32BitDirectPixels;
}

IOItemCount NvidiaGopFramebuffer30::getDisplayModeCount()
{
    return fHaveGop ? 1 : 0;
}

IOReturn NvidiaGopFramebuffer30::getDisplayModes(IODisplayModeID *allDisplayModes)
{
    if (allDisplayModes == nullptr || !fHaveGop) {
        return kIOReturnBadArgument;
    }
    allDisplayModes[0] = kGopModeID;
    return kIOReturnSuccess;
}

IOReturn NvidiaGopFramebuffer30::getInformationForDisplayMode(
    IODisplayModeID displayMode, IODisplayModeInformation *info)
{
    if (info == nullptr || displayMode != kGopModeID || !fHaveGop) {
        return kIOReturnBadArgument;
    }
    bzero(info, sizeof(*info));
    info->nominalWidth  = fWidth;
    info->nominalHeight = fHeight;
    info->refreshRate   = 60 << 16;
    info->maxDepthIndex = 0;
    info->flags = kDisplayModeValidFlag | kDisplayModeSafeFlag |
                  kDisplayModeDefaultFlag;
    return kIOReturnSuccess;
}

UInt64 NvidiaGopFramebuffer30::getPixelFormatsForDisplayMode(
    IODisplayModeID displayMode, IOIndex depth)
{
    (void)displayMode;
    (void)depth;
    return 0;
}

IOReturn NvidiaGopFramebuffer30::getPixelInformation(
    IODisplayModeID displayMode, IOIndex depth,
    IOPixelAperture aperture, IOPixelInformation *pixelInfo)
{
    if (pixelInfo == nullptr || displayMode != kGopModeID || depth != 0 ||
        !fHaveGop) {
        return kIOReturnBadArgument;
    }
    if (aperture != kIOFBSystemAperture) {
        return kIOReturnUnsupportedMode;
    }
    fillPixelInfo(pixelInfo);
    return kIOReturnSuccess;
}

IOReturn NvidiaGopFramebuffer30::getCurrentDisplayMode(IODisplayModeID *displayMode,
                                                    IOIndex *depth)
{
    if (displayMode == nullptr || depth == nullptr || !fHaveGop) {
        return kIOReturnNotReady;
    }
    *displayMode = kGopModeID;
    *depth = 0;
    return kIOReturnSuccess;
}

IOReturn NvidiaGopFramebuffer30::setDisplayMode(IODisplayModeID displayMode,
                                             IOIndex depth)
{
    // GOP is a single firmware mode. Do not program the display engine.
    if (!fHaveGop || displayMode != kGopModeID || depth != 0) {
        return kIOReturnUnsupported;
    }
    return kIOReturnSuccess;
}

IOReturn NvidiaGopFramebuffer30::getStartupDisplayMode(IODisplayModeID *displayMode,
                                                    IOIndex *depth)
{
    return getCurrentDisplayMode(displayMode, depth);
}

IOReturn NvidiaGopFramebuffer30::getTimingInfoForDisplayMode(
    IODisplayModeID displayMode, IOTimingInformation *info)
{
    if (info == nullptr || displayMode != kGopModeID || !fHaveGop) {
        return kIOReturnBadArgument;
    }
    bzero(info, sizeof(*info));
    info->appleTimingID = kIOTimingIDInvalid;
    info->flags = 0;
    return kIOReturnSuccess;
}

IOItemCount NvidiaGopFramebuffer30::getConnectionCount()
{
    return 1;
}

IOReturn NvidiaGopFramebuffer30::getAttribute(IOSelect attribute, uintptr_t *value)
{
    if (attribute == kIOHardwareCursorAttribute) {
        if (value != nullptr) {
            *value = 0;
        }
        return kIOReturnSuccess;
    }
    return super::getAttribute(attribute, value);
}

IOReturn NvidiaGopFramebuffer30::setAttribute(IOSelect attribute, uintptr_t value)
{
    if (attribute == kIOPowerAttribute) {
        if (value == 0) {
            handleEvent(kIOFBNotifyWillPowerOff);
        } else {
            handleEvent(kIOFBNotifyDidPowerOn);
        }
        return kIOReturnSuccess;
    }
    return super::setAttribute(attribute, value);
}

IOReturn NvidiaGopFramebuffer30::getAttributeForConnection(
    IOIndex connectIndex, IOSelect attribute, uintptr_t *value)
{
    if (connectIndex != 0) {
        return kIOReturnBadArgument;
    }

    switch (attribute) {
        case kConnectionFlags:
            if (value != nullptr) {
                *value = kIOConnectionBuiltIn;
            }
            return kIOReturnSuccess;
        case kConnectionEnable:
        case kConnectionCheckEnable:
            if (value != nullptr) {
                *value = fHaveGop ? 1 : 0;
            }
            return kIOReturnSuccess;
        default:
            return super::getAttributeForConnection(connectIndex, attribute, value);
    }
}

IOReturn NvidiaGopFramebuffer30::connectFlags(IOIndex connectIndex,
                                           IODisplayModeID displayMode,
                                           IOOptionBits *flags)
{
    if (connectIndex != 0 || displayMode != kGopModeID || flags == nullptr) {
        return kIOReturnBadArgument;
    }
    *flags = kDisplayModeValidFlag | kDisplayModeSafeFlag |
             kDisplayModeDefaultFlag;
    return kIOReturnSuccess;
}
