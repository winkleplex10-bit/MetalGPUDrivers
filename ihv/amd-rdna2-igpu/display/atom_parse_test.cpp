#include "atom_parse.h"

#include <stdio.h>
#include <string.h>
#include <vector>

static void WriteU16(std::vector<uint8_t> &rom, size_t off, uint16_t v)
{
	rom[off] = (uint8_t)(v & 0xFF);
	rom[off + 1] = (uint8_t)((v >> 8) & 0xFF);
}

static void WriteU32(std::vector<uint8_t> &buf, size_t off, uint32_t v)
{
	buf[off] = (uint8_t)(v & 0xFF);
	buf[off + 1] = (uint8_t)((v >> 8) & 0xFF);
	buf[off + 2] = (uint8_t)((v >> 16) & 0xFF);
	buf[off + 3] = (uint8_t)((v >> 24) & 0xFF);
}

static uint16_t PackObject(uint16_t type, uint16_t enumId, uint16_t id)
{
	return (uint16_t)((type << 12) | (enumId << 8) | id);
}

static std::vector<uint8_t> MakeRom(uint8_t contentRevision, uint16_t hdmiId, uint16_t dpId,
				    uint16_t usbcId)
{
	std::vector<uint8_t> rom(4096, 0);
	rom[0] = 0x55;
	rom[1] = 0xAA;

	const uint16_t headerOff = 0x80;
	const uint16_t masterOff = 0xC0;
	const uint16_t objOff = 0x140;
	WriteU16(rom, 0x48, headerOff);

	rom[headerOff + 4] = 'A';
	rom[headerOff + 5] = 'T';
	rom[headerOff + 6] = 'O';
	rom[headerOff + 7] = 'M';
	WriteU16(rom, headerOff + 0x20, masterOff);

	WriteU16(rom, masterOff + 4 + (22 * 2), objOff);

	rom[objOff + 2] = 1;
	rom[objOff + 3] = contentRevision;
	rom[objOff + 6] = 3;
	size_t p = objOff + 8;
	WriteU16(rom, p + 0, hdmiId);
	WriteU16(rom, p + 12, 0x0008);
	p += 16;
	WriteU16(rom, p + 0, dpId);
	WriteU16(rom, p + 12, 0x0080);
	p += 16;
	WriteU16(rom, p + 0, usbcId);
	WriteU16(rom, p + 12, 0x0200);
	return rom;
}

static std::vector<uint8_t> MakeVfct(const std::vector<uint8_t> &rom, uint16_t vendor,
				     uint16_t device)
{
	const uint32_t prefix = 76;
	const uint32_t imgHdr = 28;
	const uint32_t total = prefix + imgHdr + (uint32_t)rom.size();
	std::vector<uint8_t> vfct(total, 0);
	vfct[0] = 'V';
	vfct[1] = 'F';
	vfct[2] = 'C';
	vfct[3] = 'T';
	WriteU32(vfct, 4, total);
	WriteU32(vfct, 52, prefix);
	WriteU16(vfct, prefix + 12, vendor);
	WriteU16(vfct, prefix + 14, device);
	WriteU32(vfct, prefix + 24, (uint32_t)rom.size());
	memcpy(vfct.data() + prefix + imgHdr, rom.data(), rom.size());
	return vfct;
}

static bool ExpectHdmiDpUsbc(const AtomParseResult &result)
{
	return result.connectorCount == 3 && result.connectors[0].kind == kRaphaelConnectorHdmi &&
	       result.connectors[1].kind == kRaphaelConnectorDp &&
	       result.connectors[2].kind == kRaphaelConnectorUsbC &&
	       result.connectors[0].preferOnline && result.connectors[1].preferOnline &&
	       !result.connectors[2].preferOnline;
}

int main(void)
{
	const uint16_t hdmi = PackObject(3, 1, 0x0C);
	const uint16_t dp = PackObject(3, 1, 0x13);
	const uint16_t usbc = PackObject(3, 1, 0x17);

	std::vector<uint8_t> romV14 = MakeRom(4, hdmi, dp, usbc);
	AtomParseResult result;
	if (!AtomParseConnectors(romV14.data(), romV14.size(), &result) || !ExpectHdmiDpUsbc(result)) {
		fprintf(stderr, "v1.4 parse failed count=%u\n", result.connectorCount);
		return 1;
	}

	std::vector<uint8_t> romV15 = MakeRom(5, hdmi, dp, usbc);
	if (!AtomParseConnectors(romV15.data(), romV15.size(), &result) || !ExpectHdmiDpUsbc(result)) {
		fprintf(stderr, "v1.5 parse failed count=%u\n", result.connectorCount);
		return 1;
	}

	const uint16_t hdmiB = PackObject(3, 2, 0x0D);
	std::vector<uint8_t> romHdmiB = MakeRom(4, hdmiB, dp, usbc);
	if (!AtomParseConnectors(romHdmiB.data(), romHdmiB.size(), &result) ||
	    result.connectors[0].kind != kRaphaelConnectorHdmi) {
		fprintf(stderr, "HDMI-B mapping failed\n");
		return 1;
	}

	std::vector<uint8_t> garbage(64, 0);
	if (AtomParseConnectors(garbage.data(), garbage.size(), &result)) {
		fprintf(stderr, "garbage ROM should fail\n");
		return 1;
	}

	AtomFallbackHdmiDpUsbc(&result);
	if (!ExpectHdmiDpUsbc(result)) {
		fprintf(stderr, "fallback mismatch\n");
		return 1;
	}

	std::vector<uint8_t> vfct = MakeVfct(romV14, 0x1002, 0x164E);
	if (!AtomParseConnectorsFromVfct(vfct.data(), vfct.size(), &result) ||
	    !ExpectHdmiDpUsbc(result)) {
		fprintf(stderr, "VFCT wrap parse failed count=%u\n", result.connectorCount);
		return 1;
	}

	const uint8_t *vbios = nullptr;
	size_t vbiosLen = 0;
	uint16_t vendor = 0;
	uint16_t device = 0;
	if (!AtomExtractVbiosFromVfct(vfct.data(), vfct.size(), 76, &vbios, &vbiosLen, &vendor,
				      &device) ||
	    vendor != 0x1002 || device != 0x164E || vbiosLen != romV14.size() || !vbios ||
	    vbios[0] != 0x55) {
		fprintf(stderr, "VFCT extract failed\n");
		return 1;
	}

	std::vector<uint8_t> notVfct(128, 0);
	if (AtomParseConnectorsFromVfct(notVfct.data(), notVfct.size(), &result)) {
		fprintf(stderr, "non-VFCT table should fail\n");
		return 1;
	}

	puts("atom_parse_test ok");
	return 0;
}
