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

/*
 * ACPI VFCT (VBIOS Fetch Table) as dumped by firmware / OpenCore SysReport.
 * Layout matches linux amdgpu_acpi.c: header + UUID + VBIOSImageOffset, then
 * GOP_VBIOS_CONTENT (PCI BDF + IDs + ImageLength + 55 AA ROM).
 * Does not commit or require a VBIOS blob in git.
 */
bool AtomExtractVbiosFromVfct(const uint8_t *vfct, size_t vfctLen, uint32_t imageOffset,
			      const uint8_t **vbios, size_t *vbiosLen, uint16_t *vendorId,
			      uint16_t *deviceId);
bool AtomParseConnectorsFromVfct(const uint8_t *vfct, size_t vfctLen, AtomParseResult *out);

/*
 * Default when PCI VBIOS is not mapped: HDMI-A then DP, both preferOnline.
 * Matches locked VFCT on the 7950X3D lab (no USB-C / eDP):
 *   ihv/amd-rdna2-igpu/docs/traces/sequoia-7950x3d/vfct-atom-connectors.txt
 */
void AtomFallbackHdmiDp(AtomParseResult *out);

/* HDMI+DP+USB-C. Only for raphael_force_all=1 — this SKU’s VFCT has no USB-C. */
void AtomFallbackHdmiDpUsbc(AtomParseResult *out);
