#pragma once
extern "C"{
#include <obs.h>
#include <obs-module.h>
}
#include <string>

void register_webrtc_service();

namespace RTC{

class WHIPService{
public:
    WHIPService(obs_data_t *settings,obs_service_t *service);

    void UpdateSettings(obs_data_t *settings);
    
    const std::string& url() const{
        return url_;
    }

    const std::string& token() const{
        return bearer_token_;
    }
private:
    std::string url_;
    std::string bearer_token_;
};

}
