#include "RaphaelConnectorNub.h"
#include "../match/RaphaelController.h"

#define super IOService
OSDefineMetaClassAndStructors(RaphaelConnectorNub, IOService);

bool RaphaelConnectorNub::initWithConnector(RaphaelController *controller, uint32_t index,
					     const RaphaelConnector *spec)
{
	if (!controller || !spec || !super::init())
		return false;
	fController = controller;
	fIndex = index;
	fSpec = *spec;
	setName("RaphaelConnector");
	setProperty("connector-index", index, 32);
	setProperty("connector-kind", RaphaelConnectorKindName(spec->kind));
	return true;
}
