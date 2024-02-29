#include <output/webrtc_service.hpp>
#include <obs-service.h>

namespace RTC{

WHIPService::WHIPService(obs_data_t* settings,obs_service_t *){
    UpdateSettings(settings);
}
    
void WHIPService::UpdateSettings(obs_data_t *settings){
    url_ = obs_data_get_string(settings,"server");
    bearer_token_ = obs_data_get_string(settings,"bearer_token");
}

}

void register_webrtc_service(){
    struct obs_service_info info{};
    info.id = "whip_custom";

    info.get_name = [](void *) -> const char*{
        return obs_module_text("Service.Name");
    };
    info.create = [](obs_data_t *settings,obs_service_t *service) -> void*{
        return new RTC::WHIPService(settings,service);
    };
    info.destroy = [](void *data){
        delete static_cast<RTC::WHIPService*>(data);
    };
    info.update = [](void *data,obs_data_t *settings){
        static_cast<RTC::WHIPService*>(data)->UpdateSettings(settings);
    };
    info.get_properties = [](void *)-> obs_properties_t*{
        obs_properties_t *p = obs_properties_create();
        obs_properties_add_text(p, "server", "URL", OBS_TEXT_DEFAULT);
	    obs_properties_add_text(p, "bearer_token",
				obs_module_text("Service.BearerToken"),
				OBS_TEXT_PASSWORD);
        return p;
    };
    info.get_protocol = [](void* )->const char*{
        return "WHIP";
    };
    info.get_url = [](void *data)->const char *{
        return static_cast<RTC::WHIPService*>(data)->url().c_str();
    };
    info.get_output_type = [](void *) -> const char *{
        return "webrtc_output";
    };
    info.can_try_to_connect = [](void *data) -> bool{
        return !static_cast<RTC::WHIPService*>(data)->url().empty();
    };
    info.get_connect_info = [](void *data,uint32_t type) -> const char*{
        auto service = static_cast<RTC::WHIPService*>(data);
        switch (type)
        {
        case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
            return service->url().c_str();
        case OBS_SERVICE_CONNECT_INFO_BEARER_TOKEN:
            return service->token().c_str();
        default:
            return nullptr;
        }
    };
  	info.get_supported_video_codecs = [](void *) -> const char ** {
		static const char *video_codecs[2] = {"h264",nullptr};
        return video_codecs;
	};
	info.get_supported_audio_codecs = [](void *) -> const char ** {
        static const char *audio_codecs[2] = {"opus"};
		return audio_codecs;
	};
    obs_register_service(&info);}
