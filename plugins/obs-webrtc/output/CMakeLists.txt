
add_library(obs-webrtc MODULE)
add_library(OBS::webrtc ALIAS obs-webrtc)

target_sources(obs-webrtc PRIVATE
    webrtc_service.hpp
    webrtc_service.cc
    webrtc_output.hpp
    webrtc_output.cc
    video_source.hpp
    video_source.cc
    obs_webrtc.cc
)

target_link_libraries(obs-webrtc libwebrtc::libwebrtc OBS::libobs CURL::libcurl)

set_target_properties_obs(obs-webrtc PROPERTIES FOLDER plugings PREFIX "")