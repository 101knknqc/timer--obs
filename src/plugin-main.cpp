#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include "confidence-monitor.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("confidence-monitor", "en-US")

bool obs_module_load()
{
	ConfidenceMonitorDock::Register();
	blog(LOG_INFO, "[Confidence Monitor] Plugin loaded — version 1.0.0");
	return true;
}

void obs_module_unload()
{
	blog(LOG_INFO, "[Confidence Monitor] Plugin unloaded");
}
