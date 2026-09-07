#include "atom_parse.h"

#include <string.h>

enum {
	kAtomPciRomPtr = 0x48,
	kObjectIdMask = 0x00FF,
	kEnumIdMask = 0x0700,
	kObjectTypeMask = 0x7000,
	kObjectIdShift = 0,
	kEnumIdShift = 8,
	kObjectTypeShift = 12,
	kObjectTypeConnector = 3,
	kConnectorHdmiA = 0x0C,
	kConnectorHdmiB = 0x0D,
	kConnectorDp = 0x13,
	kConnectorEdp = 0x14,
	kConnectorUsbC = 0x17,
};

#pragma pack(push, 1)
struct AtomCommonHeader {
	uint16_t structureSize;
	uint8_t formatRevision;
	uint8_t contentRevision;
};

struct AtomRomHeaderV22 {
	AtomCommonHeader tableHeader;
	uint8_t atomBiosString[4];
	uint16_t biosSegmentAddress;
	uint16_t protectedModeOffset;
	uint16_t configFilenameOffset;
	uint16_t crcBlockOffset;
	uint16_t vbiosBootupMessageOffset;
	uint16_t int10Offset;
	uint16_t pciBusDevInitCode;
	uint16_t ioBaseAddress;
	uint16_t subsystemVendorId;
	uint16_t subsystemId;
	uint16_t pciInfoOffset;
	uint16_t masterHwFunctionOffset;
	uint16_t masterDataTableOffset;
	uint16_t reserved;
	uint32_t pspDirTableOffset;
};

struct AtomMasterListV21 {
	uint16_t utilityPipeline;
	uint16_t multimediaInfo;
	uint16_t smcDpmInfo;
	uint16_t swDatatable3;
	uint16_t firmwareInfo;
	uint16_t swDatatable5;
	uint16_t lcdInfo;
	uint16_t swDatatable7;
	uint16_t smuInfo;
	uint16_t swDatatable9;
	uint16_t swDatatable10;
	uint16_t vramUsageByFirmware;
	uint16_t gpioPinLut;
	uint16_t swDatatable13;
	uint16_t gfxInfo;
	uint16_t powerplayInfo;
	uint16_t swDatatable16;
	uint16_t swDatatable17;
	uint16_t swDatatable18;
	uint16_t swDatatable19;
	uint16_t swDatatable20;
	uint16_t swDatatable21;
	uint16_t displayObjectInfo;
};

struct AtomMasterDataTableV21 {
	AtomCommonHeader tableHeader;
	AtomMasterListV21 list;
};

struct AtomDisplayPathV2 {
	uint16_t displayObjId;
	uint16_t dispRecordOffset;
	uint16_t encoderObjId;
	uint16_t extEncoderObjId;
	uint16_t encoderRecordOffset;
	uint16_t extEncoderRecordOffset;
	uint16_t deviceTag;
	uint8_t priorityId;
	uint8_t reserved;
};

struct AtomDisplayPathV3 {
	uint16_t displayObjId;
	uint16_t dispRecordOffset;
	uint16_t encoderObjId;
	uint16_t reserved1;
	uint16_t reserved2;
	uint16_t reserved3;
	uint16_t deviceTag;
	uint16_t reserved4;
};

struct AtomDisplayObjectInfo {
	AtomCommonHeader tableHeader;
	uint16_t supportedDevices;
	uint8_t numberOfPath;
	uint8_t reserved;
};
#pragma pack(pop)

static bool InRange(size_t off, size_t need, size_t len)
{
	return off <= len && len - off >= need;
}

static uint16_t ReadU16(const uint8_t *p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static RaphaelConnectorKind KindFromObjectId(uint16_t objectId)
{
	const uint16_t type = (objectId & kObjectTypeMask) >> kObjectTypeShift;
	const uint16_t id = (objectId & kObjectIdMask) >> kObjectIdShift;
	if (type != 0 && type != kObjectTypeConnector)
		return kRaphaelConnectorUnknown;
	switch (id) {
	case kConnectorHdmiA:
	case kConnectorHdmiB:
		return kRaphaelConnectorHdmi;
	case kConnectorDp:
		return kRaphaelConnectorDp;
	case kConnectorUsbC:
		return kRaphaelConnectorUsbC;
	case kConnectorEdp:
		return kRaphaelConnectorEdp;
	default:
		return kRaphaelConnectorUnknown;
	}
}

static bool AppendConnector(AtomParseResult *out, uint16_t objectId, uint16_t deviceTag)
{
	const RaphaelConnectorKind kind = KindFromObjectId(objectId);
	if (kind == kRaphaelConnectorUnknown)
		return false;
	if (out->connectorCount >= kRaphaelMaxConnectors)
		return false;
	for (uint32_t i = 0; i < out->connectorCount; i++) {
		if (out->connectors[i].objectId == objectId)
			return false;
	}
	RaphaelConnector *c = &out->connectors[out->connectorCount];
	c->kind = kind;
	c->enumId = (uint8_t)((objectId & kEnumIdMask) >> kEnumIdShift);
	c->deviceTag = deviceTag;
	c->objectId = objectId;
	c->preferOnline = (kind == kRaphaelConnectorHdmi || kind == kRaphaelConnectorDp);
	out->connectorCount++;
	return true;
}

static void ApplyPriority(AtomParseResult *out)
{
	if (out->connectorCount == 0)
		return;
	for (uint32_t i = 0; i < out->connectorCount; i++) {
		if (out->connectors[i].preferOnline)
			return;
	}
	out->connectors[0].preferOnline = true;
}

void AtomFallbackHdmiDpUsbc(AtomParseResult *out)
{
	if (!out)
		return;
	memset(out, 0, sizeof(*out));
	out->connectorCount = 3;
	out->connectors[0].kind = kRaphaelConnectorHdmi;
	out->connectors[0].preferOnline = true;
	out->connectors[1].kind = kRaphaelConnectorDp;
	out->connectors[1].preferOnline = true;
	out->connectors[2].kind = kRaphaelConnectorUsbC;
	out->connectors[2].preferOnline = false;
}

bool AtomParseConnectors(const uint8_t *rom, size_t romLen, AtomParseResult *out)
{
	if (!rom || !out || romLen < 0x50)
		return false;
	memset(out, 0, sizeof(*out));
	if (rom[0] != 0x55 || rom[1] != 0xAA)
		return false;

	const uint16_t headerOff = ReadU16(rom + kAtomPciRomPtr);
	if (!InRange(headerOff, sizeof(AtomRomHeaderV22), romLen))
		return false;

	AtomRomHeaderV22 header;
	memcpy(&header, rom + headerOff, sizeof(header));
	if (memcmp(header.atomBiosString, "ATOM", 4) != 0)
		return false;

	const uint16_t masterOff = header.masterDataTableOffset;
	if (!InRange(masterOff, sizeof(AtomMasterDataTableV21), romLen))
		return false;

	AtomMasterDataTableV21 master;
	memcpy(&master, rom + masterOff, sizeof(master));
	const uint16_t objOff = master.list.displayObjectInfo;
	if (objOff == 0 || !InRange(objOff, sizeof(AtomDisplayObjectInfo), romLen))
		return false;

	AtomDisplayObjectInfo info;
	memcpy(&info, rom + objOff, sizeof(info));
	if (info.numberOfPath == 0)
		return false;

	const uint8_t *paths = rom + objOff + sizeof(AtomDisplayObjectInfo);
	const size_t pathsOff = objOff + sizeof(AtomDisplayObjectInfo);
	if (info.tableHeader.contentRevision >= 5) {
		const size_t need = (size_t)info.numberOfPath * sizeof(AtomDisplayPathV3);
		if (!InRange(pathsOff, need, romLen))
			return false;
		for (uint8_t i = 0; i < info.numberOfPath; i++) {
			AtomDisplayPathV3 path;
			memcpy(&path, paths + i * sizeof(AtomDisplayPathV3), sizeof(path));
			if (path.displayObjId == 0)
				continue;
			AppendConnector(out, path.displayObjId, path.deviceTag);
		}
	} else {
		const size_t need = (size_t)info.numberOfPath * sizeof(AtomDisplayPathV2);
		if (!InRange(pathsOff, need, romLen))
			return false;
		for (uint8_t i = 0; i < info.numberOfPath; i++) {
			AtomDisplayPathV2 path;
			memcpy(&path, paths + i * sizeof(AtomDisplayPathV2), sizeof(path));
			if (path.displayObjId == 0)
				continue;
			AppendConnector(out, path.displayObjId, path.deviceTag);
		}
	}

	ApplyPriority(out);
	return out->connectorCount > 0;
}
