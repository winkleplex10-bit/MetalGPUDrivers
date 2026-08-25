#!/usr/bin/env python3
"""Stamp GOP framebuffer kexts for NvidiaMetal16/20/30/40 from NvidiaMetal50."""

from pathlib import Path
import shutil

ROOT = Path("/Volumes/HDD RAID 4TB/MetalGPUDrivers")
SRC = ROOT / "NvidiaMetal50"

SERIES = {
    "16": {
        "arch": "Turing TU11x",
        "cards": "GTX 1630 / 1650 / 1660",
        "ids": [
            (0x1F0A, "GeForce GTX 1650"),
            (0x1F82, "GeForce GTX 1650"),
            (0x1F83, "GeForce GTX 1630"),
            (0x1F91, "GeForce GTX 1650"),
            (0x1F94, "GeForce GTX 1650"),
            (0x1F95, "GeForce GTX 1650 Ti"),
            (0x1F96, "GeForce GTX 1650"),
            (0x1F97, "GeForce GTX 1650"),
            (0x1F98, "GeForce GTX 1650"),
            (0x1F99, "GeForce GTX 1650"),
            (0x1F9C, "GeForce GTX 1650"),
            (0x1F9D, "GeForce GTX 1650"),
            (0x1F9F, "GeForce GTX 1650 Ti"),
            (0x1FDD, "GeForce GTX 1650"),
            (0x2182, "GeForce GTX 1660 Ti"),
            (0x2184, "GeForce GTX 1660"),
            (0x2187, "GeForce GTX 1650 SUPER"),
            (0x2188, "GeForce GTX 1650"),
            (0x2189, "GeForce GTX 1660 SUPER"),
            (0x2191, "GeForce GTX 1660 Ti"),
            (0x2192, "GeForce GTX 1650 Ti"),
            (0x21C4, "GeForce GTX 1660 SUPER"),
            (0x21D1, "GeForce GTX 1660 Ti"),
        ],
    },
    "20": {
        "arch": "Turing TU10x",
        "cards": "RTX 2060 / 2070 / 2080",
        "ids": [
            (0x1E04, "GeForce RTX 2080 Ti"),
            (0x1E07, "GeForce RTX 2080 Ti"),
            (0x1E81, "GeForce RTX 2080 SUPER"),
            (0x1E82, "GeForce RTX 2080"),
            (0x1E84, "GeForce RTX 2070 SUPER"),
            (0x1E87, "GeForce RTX 2080"),
            (0x1E89, "GeForce RTX 2060"),
            (0x1E90, "GeForce RTX 2080"),
            (0x1E91, "GeForce RTX 2070 Super"),
            (0x1E93, "GeForce RTX 2080 Super"),
            (0x1EC2, "GeForce RTX 2070 SUPER"),
            (0x1EC7, "GeForce RTX 2070 SUPER"),
            (0x1ED0, "GeForce RTX 2080"),
            (0x1ED1, "GeForce RTX 2070 Super"),
            (0x1ED3, "GeForce RTX 2080 Super"),
            (0x1F02, "GeForce RTX 2070"),
            (0x1F03, "GeForce RTX 2060"),
            (0x1F06, "GeForce RTX 2060 SUPER"),
            (0x1F07, "GeForce RTX 2070"),
            (0x1F08, "GeForce RTX 2060"),
            (0x1F10, "GeForce RTX 2070"),
            (0x1F11, "GeForce RTX 2060"),
            (0x1F12, "GeForce RTX 2060"),
            (0x1F14, "GeForce RTX 2070"),
            (0x1F15, "GeForce RTX 2060"),
            (0x1F42, "GeForce RTX 2060 SUPER"),
            (0x1F47, "GeForce RTX 2060 SUPER"),
            (0x1F50, "GeForce RTX 2070"),
            (0x1F51, "GeForce RTX 2060"),
            (0x1F54, "GeForce RTX 2070"),
            (0x1F55, "GeForce RTX 2060"),
        ],
    },
    "30": {
        "arch": "Ampere GA10x",
        "cards": "RTX 3050 / 3060 / 3070 / 3080 / 3090",
        "ids": [
            (0x2203, "GeForce RTX 3090 Ti"),
            (0x2204, "GeForce RTX 3090"),
            (0x2206, "GeForce RTX 3080"),
            (0x2207, "GeForce RTX 3070 Ti"),
            (0x2208, "GeForce RTX 3080 Ti"),
            (0x220A, "GeForce RTX 3080"),
            (0x2216, "GeForce RTX 3080"),
            (0x2414, "GeForce RTX 3060 Ti"),
            (0x2420, "GeForce RTX 3080 Ti Laptop"),
            (0x2460, "GeForce RTX 3080 Ti Laptop"),
            (0x2482, "GeForce RTX 3070 Ti"),
            (0x2484, "GeForce RTX 3070"),
            (0x2486, "GeForce RTX 3060 Ti"),
            (0x2487, "GeForce RTX 3060"),
            (0x2488, "GeForce RTX 3070"),
            (0x2489, "GeForce RTX 3060 Ti"),
            (0x249C, "GeForce RTX 3080 Laptop"),
            (0x249D, "GeForce RTX 3070 Laptop"),
            (0x24A0, "GeForce RTX 3060 Laptop"),
            (0x24C7, "GeForce RTX 3060"),
            (0x24C9, "GeForce RTX 3060 Ti"),
            (0x24DC, "GeForce RTX 3080 Laptop"),
            (0x24DD, "GeForce RTX 3070 Laptop"),
            (0x24E0, "GeForce RTX 3070 Ti Laptop"),
            (0x2503, "GeForce RTX 3060"),
            (0x2504, "GeForce RTX 3060"),
            (0x2507, "GeForce RTX 3050"),
            (0x2508, "GeForce RTX 3050 OEM"),
            (0x2520, "GeForce RTX 3060 Laptop"),
            (0x2521, "GeForce RTX 3060 Laptop"),
            (0x2523, "GeForce RTX 3050 Ti Laptop"),
            (0x2544, "GeForce RTX 3060"),
            (0x2560, "GeForce RTX 3060 Laptop"),
            (0x2563, "GeForce RTX 3050 Ti Laptop"),
            (0x2582, "GeForce RTX 3050"),
            (0x2584, "GeForce RTX 3050"),
            (0x25A0, "GeForce RTX 3060 Laptop"),
            (0x25A2, "GeForce RTX 3050 Laptop"),
            (0x25A5, "GeForce RTX 3050 Laptop"),
            (0x25A7, "GeForce RTX 2050"),
            (0x25A9, "GeForce RTX 2050"),
            (0x25AB, "GeForce RTX 3050 4GB Laptop"),
            (0x25AC, "GeForce RTX 3050 6GB Laptop"),
            (0x25AD, "GeForce RTX 2050"),
            (0x25E0, "GeForce RTX 3050 Ti Laptop"),
            (0x25E2, "GeForce RTX 3050 Laptop"),
            (0x25E5, "GeForce RTX 3050 Laptop"),
            (0x25EC, "GeForce RTX 3050 6GB Laptop"),
            (0x25ED, "GeForce RTX 2050"),
            (0x2822, "GeForce RTX 3050 A Laptop"),
            (0x28A3, "GeForce RTX 3050 A Laptop"),
            (0x28E3, "GeForce RTX 3050 A Laptop"),
        ],
    },
    "40": {
        "arch": "Ada Lovelace AD10x",
        "cards": "RTX 4050 / 4060 / 4070 / 4080 / 4090",
        "ids": [
            (0x2684, "GeForce RTX 4090"),
            (0x2685, "GeForce RTX 4090 D"),
            (0x2689, "GeForce RTX 4070 Ti SUPER"),
            (0x2702, "GeForce RTX 4080 SUPER"),
            (0x2704, "GeForce RTX 4080"),
            (0x2705, "GeForce RTX 4070 Ti SUPER"),
            (0x2709, "GeForce RTX 4070"),
            (0x2717, "GeForce RTX 4090 Laptop"),
            (0x2757, "GeForce RTX 4090 Laptop"),
            (0x2782, "GeForce RTX 4070 Ti"),
            (0x2783, "GeForce RTX 4070 SUPER"),
            (0x2786, "GeForce RTX 4070"),
            (0x2788, "GeForce RTX 4060 Ti"),
            (0x27A0, "GeForce RTX 4080 Laptop"),
            (0x27E0, "GeForce RTX 4080 Laptop"),
            (0x2803, "GeForce RTX 4060 Ti"),
            (0x2805, "GeForce RTX 4060 Ti"),
            (0x2808, "GeForce RTX 4060"),
            (0x2820, "GeForce RTX 4070 Laptop"),
            (0x2860, "GeForce RTX 4070 Laptop"),
            (0x2882, "GeForce RTX 4060"),
            (0x28A0, "GeForce RTX 4060 Laptop"),
            (0x28A1, "GeForce RTX 4050 Laptop"),
            (0x28E0, "GeForce RTX 4060 Laptop"),
            (0x28E1, "GeForce RTX 4050 Laptop"),
        ],
    },
}


def pci_header(num: str, meta: dict) -> str:
    cases = "\n".join(
        f'        case 0x{did:04X}: return "{name}";' for did, name in meta["ids"]
    )
    return f"""//
// Known {meta['cards']} PCI IDs for logging. Matching is vendor+class, not this list.
// IDs from open-gpu-kernel-modules 610.57.04 README ({meta['arch']}).
//

#ifndef NV_METAL{num}_PCI_IDS_H
#define NV_METAL{num}_PCI_IDS_H

#include <libkern/OSTypes.h>

static inline const char *NvidiaChipName(UInt16 deviceId)
{{
    switch (deviceId) {{
{cases}
        default:     return "NVIDIA GPU";
    }}
}}

#endif
"""


def rewrite(text: str, num: str) -> str:
    cls = f"NvidiaGopFramebuffer{num}"
    text = text.replace("NvidiaGopFramebuffer", cls)
    text = text.replace("NvidiaMetal50", f"NvidiaMetal{num}")
    text = text.replace("NV_METAL50", f"NV_METAL{num}")
    text = text.replace("com.metalgpudrivers.NvidiaMetal50", f"com.metalgpudrivers.NvidiaMetal{num}")
    return text


def stamp(num: str, meta: dict) -> None:
    dest = ROOT / f"NvidiaMetal{num}"
    dest.mkdir(exist_ok=True)
    gitkeep = dest / ".gitkeep"
    if gitkeep.exists():
        gitkeep.unlink()

    mapping = {
        "Makefile": SRC / "Makefile",
        "README.md": SRC / "README.md",
        ".gitignore": SRC / ".gitignore",
        "kext/NvidiaFB/Info.plist": SRC / "kext/NvidiaFB/Info.plist",
        "kext/NvidiaFB/NvidiaGopFramebuffer.hpp": SRC / "kext/NvidiaFB/NvidiaGopFramebuffer.hpp",
        "kext/NvidiaFB/NvidiaGopFramebuffer.cpp": SRC / "kext/NvidiaFB/NvidiaGopFramebuffer.cpp",
        "docs/INSTALL.md": SRC / "docs/INSTALL.md",
        "docs/GOP-FRAMEBUFFER.md": SRC / "docs/GOP-FRAMEBUFFER.md",
        "tools/disable-legacy-nv.sh": SRC / "tools/disable-legacy-nv.sh",
        "tools/dump-nv-ioreg.sh": SRC / "tools/dump-nv-ioreg.sh",
        "firmware/README.md": SRC / "firmware/README.md",
    }

    for rel, src in mapping.items():
        out = dest / rel
        out.parent.mkdir(parents=True, exist_ok=True)
        text = rewrite(src.read_text(), num)
        # Keep on-disk source names; only IOClass / bundle / log prefix change.
        text = text.replace(f'#include "NvidiaGopFramebuffer{num}.hpp"', '#include "NvidiaGopFramebuffer.hpp"')
        if rel == "Makefile":
            text = text.replace(f"NvidiaGopFramebuffer{num}.cpp", "NvidiaGopFramebuffer.cpp")
            text = text.replace(f"NvidiaGopFramebuffer{num}.hpp", "NvidiaGopFramebuffer.hpp")
            text = text.replace(f"NvidiaGopFramebuffer{num}.o", "NvidiaGopFramebuffer.o")
        if rel == "README.md":
            note = (
                f"**Series folder:** {meta['arch']} ({meta['cards']}). "
                "The GOP kext still matches every NVIDIA VGA device (vendor `10DE`, "
                "class `0x03`) so an unknown ID cannot fall through to Kepler "
                "`NVDAStartup`. Inject **only one** `NvidiaMetalXX.kext`.\n\n"
            )
            # insert after first heading block
            parts = text.split("\n\n", 1)
            if len(parts) == 2:
                text = parts[0] + "\n\n" + note + parts[1]
            else:
                text = note + text
        out.write_text(text)
        if rel.endswith(".sh"):
            out.chmod(0o755)

    (dest / "kext/NvidiaFB/NvidiaPciIds.h").write_text(pci_header(num, meta))
    print(f"stamped {dest}")


def main() -> None:
    for num, meta in SERIES.items():
        stamp(num, meta)


if __name__ == "__main__":
    main()
