#pragma once

#include "RaphaelIds.h"

#include <stddef.h>
#include <stdint.h>

struct AtomParseResult {
	uint32_t connectorCount;
	RaphaelConnector connectors[kRaphaelMaxConnectors];
};

/*
 * Parse ATOM displayObjectInfo paths from a PCI option ROM image.
 *
 * Layout matches linux atomfirmware.h:
 *   PCI ROM ptr @ 0x48 → atom_rom_header_v2_2
 *   masterdatatable_offset → atom_master_data_table_v2_1
 *   list.displayobjectinfo (index 22) → display_object_info_table_v1_4 / v1_5
 *
 * Connector object IDs from amdgpu ObjectID.h:
 *   HDMI-A 0x0C, HDMI-B 0x0D, DP 0x13, eDP 0x14, USB-C 0x17
 */
bool AtomParseConnectors(const uint8_t *rom, size_t romLen, AtomParseResult *out);

/* HDMI + DP online, USB-C enumerated but not preferred — used when VBIOS is unmapped. */
void AtomFallbackHdmiDpUsbc(AtomParseResult *out);
