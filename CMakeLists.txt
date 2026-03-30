cmake_minimum_required(VERSION 3.16)
project(confidence-monitor VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Qt6 ──────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)
qt_standard_project_setup()

# ── OBS paths ─────────────────────────────────────────────────
# CI (from source build) passes all 5 variables explicitly:
#   OBS_SOURCE_DIR  = obs-studio source root
#   OBS_CONFIG_DIR  = dir containing generated obsconfig.h
#   OBS_FE_INCLUDE  = dir containing obs-frontend-api.h
#   OBS_LIB_DIR     = dir containing obs.lib
#   OBS_FE_LIB_DIR  = dir containing obs-frontend-api.lib
#
# Local build: set OBS_INSTALL_DIR (default: Program Files/obs-studio)

if(NOT DEFINED OBS_SOURCE_DIR)
  set(OBS_INSTALL_DIR "C:/Program Files/obs-studio" CACHE PATH "OBS install")
  set(OBS_SOURCE_DIR  "${OBS_INSTALL_DIR}")
  set(OBS_CONFIG_DIR  "${OBS_INSTALL_DIR}/include")
  set(OBS_FE_INCLUDE  "${OBS_INSTALL_DIR}/include")
  set(OBS_LIB_DIR     "${OBS_INSTALL_DIR}/bin/64bit")
  set(OBS_FE_LIB_DIR  "${OBS_INSTALL_DIR}/bin/64bit")
endif()

if(NOT DEFINED OBS_CONFIG_DIR)
  set(OBS_CONFIG_DIR "${OBS_SOURCE_DIR}/build/config")
endif()
if(NOT DEFINED OBS_FE_INCLUDE)
  # Try new location (OBS 32+) then old location
  if(EXISTS "${OBS_SOURCE_DIR}/frontend/api/obs-frontend-api.h")
    set(OBS_FE_INCLUDE "${OBS_SOURCE_DIR}/frontend/api")
  else()
    set(OBS_FE_INCLUDE "${OBS_SOURCE_DIR}/UI/obs-frontend-api")
  endif()
endif()
if(NOT DEFINED OBS_LIB_DIR)
  set(OBS_LIB_DIR "${OBS_SOURCE_DIR}/build/libobs/Release")
endif()
if(NOT DEFINED OBS_FE_LIB_DIR)
  set(OBS_FE_LIB_DIR "${OBS_SOURCE_DIR}/build/frontend/api/Release")
endif()

set(OBS_INCLUDE_DIR "${OBS_SOURCE_DIR}/libobs")

find_library(OBS_LIB    NAMES obs libobs           PATHS "${OBS_LIB_DIR}"    NO_DEFAULT_PATH REQUIRED)
find_library(OBS_FE_LIB NAMES obs-frontend-api     PATHS "${OBS_FE_LIB_DIR}" NO_DEFAULT_PATH REQUIRED)

message(STATUS "OBS include    : ${OBS_INCLUDE_DIR}")
message(STATUS "OBS FE include : ${OBS_FE_INCLUDE}")
message(STATUS "OBS config dir : ${OBS_CONFIG_DIR}")
message(STATUS "OBS lib        : ${OBS_LIB}")
message(STATUS "OBS FE lib     : ${OBS_FE_LIB}")

# ── Sources ───────────────────────────────────────────────────
add_library(confidence-monitor MODULE
    src/plugin-main.cpp
    src/confidence-monitor.cpp
    src/confidence-monitor-dock.cpp
    src/confidence-monitor.hpp
    src/resources.qrc
)

target_include_directories(confidence-monitor PRIVATE
    ${OBS_INCLUDE_DIR}
    ${OBS_FE_INCLUDE}
    ${OBS_CONFIG_DIR}
    src
)

target_link_libraries(confidence-monitor PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Multimedia
    ${OBS_LIB} ${OBS_FE_LIB}
)

set_target_properties(confidence-monitor PROPERTIES PREFIX "" OUTPUT_NAME "confidence-monitor")

# ── Install ───────────────────────────────────────────────────
set(OBS_PLUGIN_DIR "$ENV{APPDATA}/obs-studio/plugins" CACHE PATH "OBS plugin dir")
install(TARGETS confidence-monitor
    RUNTIME DESTINATION "${OBS_PLUGIN_DIR}/confidence-monitor/bin/64bit"
    LIBRARY DESTINATION "${OBS_PLUGIN_DIR}/confidence-monitor/bin/64bit"
)
install(DIRECTORY data/ DESTINATION "${OBS_PLUGIN_DIR}/confidence-monitor/data")
