#pragma once

#include <stdint.h>

enum {
	kRaphaelVendorId = 0x1002,
	kRaphaelDeviceId = 0x164E,
	kRaphaelPciMatch = 0x164E1002,
	kRaphaelMaxConnectors = 6,
	kRaphaelMaxModes = 8,
	kRaphaelDefaultWidth = 3840,
	kRaphaelDefaultHeight = 2160,
	kRaphaelDefaultRefreshHz = 60,
};

enum RaphaelConnectorKind : uint8_t {
	kRaphaelConnectorUnknown = 0,
	kRaphaelConnectorHdmi = 1,
	kRaphaelConnectorDp = 2,
	kRaphaelConnectorUsbC = 3,
	kRaphaelConnectorEdp = 4,
	kRaphaelConnectorGop = 5,
};

struct RaphaelConnector {
	RaphaelConnectorKind kind;
	uint8_t enumId;
	uint16_t deviceTag;
	uint16_t objectId;
	bool preferOnline;
};

static const char kRaphaelMetalPluginName[] = "RaphaelMTLDriver";
static const char kRaphaelMetalPluginClass[] = "RaphaelMTLDriver";
static const char kRaphaelModelName[] = "AMD Radeon Graphics (Raphael)";

static inline const char *RaphaelConnectorKindName(RaphaelConnectorKind kind)
{
	switch (kind) {
	case kRaphaelConnectorHdmi:
		return "HDMI";
	case kRaphaelConnectorDp:
		return "DP";
	case kRaphaelConnectorUsbC:
		return "USB-C";
	case kRaphaelConnectorEdp:
		return "eDP";
	case kRaphaelConnectorGop:
		return "GOP";
	case kRaphaelConnectorUnknown:
		return "unknown";
	}
	return "unknown";
}
