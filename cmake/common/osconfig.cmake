# OBS CMake operating system bootstrap module

include_guard(GLOBAL)

# Set minimum CMake version specific to host operating system, add OS-specific module directory to default search paths,
# and set helper variables for OS detection in other CMake list files.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
  set(CMAKE_C_EXTENSIONS FALSE)
  set(CMAKE_CXX_EXTENSIONS FALSE)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows")
  set(OS_WINDOWS TRUE)
  set(_os_prebuilds "windows")    
  set(_os_arch "x86_64")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  set(CMAKE_C_EXTENSIONS FALSE)
  set(CMAKE_CXX_EXTENSIONS FALSE)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos")
  set(_os_prebuilds "mac")    
  set(_os_arch "arm64")
  set(OS_MACOS TRUE)
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD|OpenBSD")
  set(CMAKE_CXX_EXTENSIONS FALSE)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/linux")
  string(TOUPPER "${CMAKE_HOST_SYSTEM_NAME}" _SYSTEM_NAME_U)
  set(OS_${_SYSTEM_NAME_U} TRUE)
endif()
