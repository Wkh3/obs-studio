include(CMakePrintHelpers)

option(ENABLE_WEBRTC "Enable WebRTC Output support" ON)
if(NOT ENABLE_WEBRTC)
  target_disable(obs-webrtc)
  return()
endif()

find_package(libwebrtc REQUIRED)

if(libwebrtc_FOUND)
    cmake_print_properties(TARGETS "libwebrtc::libwebrtc" PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
        IMPORTED_LOCATION
    )
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(output)