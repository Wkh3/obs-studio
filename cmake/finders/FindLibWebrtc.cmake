include(FindPackageHandleStandardArgs)

find_path(
    WEBRTC_INCLUDE_DIR
    NAMES "include/api/peer_connection_interface.h"
    PATHS ${dependencies_install_path}/libwebrtc
)

find_library(
    WEBRTC_LIBRARY
    NAMES libwebrtc.a
    PATHS ${dependencies_install_path}/libwebrtc/lib
)

message(STATUS "WEBRTC_INCLUDE_DIR = ${WEBRTC_INCLUDE_DIR}")

find_package_handle_standard_args(libwebrtc WEBRTC_INCLUDE_DIR WEBRTC_LIBRARY)

mark_as_advanced(WEBRTC_INCLUDE_DIR WEBRTC_LIBRARY)

set(LIBWEBRTC_INCLUDE_DIR ${WEBRTC_INCLUDE_DIR}/include 
                          ${WEBRTC_INCLUDE_DIR}/include/third_party
                          ${WEBRTC_INCLUDE_DIR}/include/third_party/abseil-cpp
                          ${WEBRTC_INCLUDE_DIR}/include/third_party/jsoncpp/source/include
                          )

if(libwebrtc_FOUND)
    add_library(libwebrtc::libwebrtc STATIC IMPORTED)
    set_target_properties(libwebrtc::libwebrtc PROPERTIES 
        IMPORTED_LOCATION ${WEBRTC_LIBRARY}
    )

    target_compile_definitions(libwebrtc::libwebrtc INTERFACE
        -DRTC_DCHECK_IS_ON=0
        -DWEBRTC_USE_H264
        $<$<NOT:$<PLATFORM_ID:Windows>>:WEBRTC_POSIX>
        $<$<PLATFORM_ID:Windows>:WEBRTC_WIN>
        $<$<PLATFORM_ID:Windows>:NOMINMAX>
        $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
        $<$<PLATFORM_ID:Darwin>:WEBRTC_MAC>
    )

    target_include_directories(libwebrtc::libwebrtc INTERFACE ${LIBWEBRTC_INCLUDE_DIR})
    target_link_libraries(libwebrtc::libwebrtc INTERFACE 
    "$<LINK_LIBRARY:FRAMEWORK,AudioUnit.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreAudio.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreMedia.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreVideo.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,IOSurface.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,AudioToolBox.framework>"
    "$<LINK_LIBRARY:WEAK_FRAMEWORK,ScreenCaptureKit.framework>")
    
endif()