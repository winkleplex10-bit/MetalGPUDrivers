#include "RaphaelAccelerator.h"
#include "../match/RaphaelController.h"

#include "RaphaelIds.h"

#include <IOKit/IOLib.h>

#define super IOService
OSDefineMetaClassAndStructors(RaphaelAccelerator, IOService);

bool RaphaelAccelerator::startWithController(RaphaelController *controller)
{
	if (!controller)
		return false;
	if (!init())
		return false;
	if (!attach(controller))
		return false;
	if (!start(controller)) {
		detach(controller);
		return false;
	}
	return true;
}

bool RaphaelAccelerator::start(IOService *provider)
{
	if (!super::start(provider))
		return false;

	RaphaelController *controller = OSDynamicCast(RaphaelController, provider);
	setName("RaphaelAccelerator");
	setProperty("RaphaelAccelStatus",
		    "gop-wrap; no GFX ring; MetalPluginName off unless raphael_metal=1");
	setProperty("RaphaelPhase", "R2-gop-wrap");
	setProperty("model", kRaphaelModelName);

	if (controller && controller->metalEnabled()) {
		/*
		 * Off by default. Advertising MetalPluginName without a gfx1036
		 * plugin makes Metal.framework dlopen a missing bundle.
		 */
		setProperty("MetalPluginName", kRaphaelMetalPluginName);
		setProperty("MetalPluginClassName", kRaphaelMetalPluginClass);
		IOLog("RaphaelAccelerator: MetalPluginName advertised (raphael_metal=1); "
		      "plugin is a stub, not QE/Metal\n");
	}

	registerService();
	return true;
}

void RaphaelAccelerator::stop(IOService *provider)
{
	super::stop(provider);
}
