#pragma once

#include <stdint.h>

// Lab-frozen Phase 0/1 board: GeForce RTX 5080 (GB203 consumer Blackwell).
// Cited: open-gpu-kernel-modules Compatible GPUs table (DID 2C02);
// measured on this host as IOPCIDevice GFX0@0 (vendor-id / device-id little-endian).

enum {
	kNvidiaVendorId = 0x10DE,
	kNvidiaDeviceId5080 = 0x2C02,
	kNvidiaPciMatch5080 = 0x2C0210DE,
	kNvidiaHdauDeviceId5080 = 0x22E9, // function 0.1 audio; do not claim
	kNvidiaMaxBars = 8,
};

static const char kNvidiaModelName5080[] = "GeForce RTX 5080 (unaccelerated)";
static const char kNvidiaArchName[] = "Blackwell GB20x";
static const char kNvidiaPhaseEnumerate[] = "N1-enumerate";
