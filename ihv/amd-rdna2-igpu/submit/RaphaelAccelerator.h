#pragma once

#include <IOKit/IOService.h>

class RaphaelController;

class RaphaelAccelerator : public IOService {
	OSDeclareDefaultStructors(RaphaelAccelerator);

public:
	bool startWithController(RaphaelController *controller);
	virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
	virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
};
