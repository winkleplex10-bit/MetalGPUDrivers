#pragma once

#include "RaphaelIds.h"

#include <IOKit/IOService.h>

class RaphaelController;

class RaphaelConnectorNub : public IOService {
	OSDeclareDefaultStructors(RaphaelConnectorNub);

public:
	bool initWithConnector(RaphaelController *controller, uint32_t index,
			       const RaphaelConnector *spec);
	RaphaelController *controller() const { return fController; }
	uint32_t connectorIndex() const { return fIndex; }
	const RaphaelConnector &spec() const { return fSpec; }

private:
	RaphaelController *fController;
	uint32_t fIndex;
	RaphaelConnector fSpec;
};
