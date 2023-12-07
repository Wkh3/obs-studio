#include <obs-module.h>
#include <output/webrtc_output.hpp>
#include <output/webrtc_service.hpp>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-webrtc", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "OBS WebRTC Native module";
}

bool obs_module_load()
{
	register_webrtc_output();
	register_webrtc_service();

	return true;
}
