cmake_minimum_required(VERSION 3.16)
project(confidence-monitor VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Qt6 ──────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)
qt_standard_project_setup()

# ── OBS paths ─────────────────────────────────────────────────
# Usage options:
#   A) OBS_SOURCE_DIR  = path to obs-studio source clone (headers + separate lib dirs)
#      + OBS_LIB_DIR   = dir containing obs.lib
#      + OBS_FE_LIB_DIR= dir containing obs-frontend-api.lib
#   B) OBS_INSTALL_DIR = path to installed OBS (C:/Program Files/obs-studio)

if(DEFINED OBS_SOURCE_DIR)
    set(OBS_INCLUDE_DIR     "${OBS_SOURCE_DIR}/libobs")
    set(OBS_FE_INCLUDE_DIR  "${OBS_SOURCE_DIR}/UI/obs-frontend-api")
else()
    set(OBS_INSTALL_DIR "C:/Program Files/obs-studio" CACHE PATH "OBS install dir")
    set(OBS_SOURCE_DIR      "${OBS_INSTALL_DIR}")
    set(OBS_INCLUDE_DIR     "${OBS_INSTALL_DIR}/include")
    set(OBS_FE_INCLUDE_DIR  "${OBS_INSTALL_DIR}/include")
    set(OBS_LIB_DIR         "${OBS_INSTALL_DIR}/bin/64bit")
    set(OBS_FE_LIB_DIR      "${OBS_INSTALL_DIR}/bin/64bit")
endif()

# Search dirs for .lib files
if(NOT DEFINED OBS_LIB_DIR)
    set(OBS_LIB_DIR    "${OBS_SOURCE_DIR}/build/libobs")
endif()
if(NOT DEFINED OBS_FE_LIB_DIR)
    set(OBS_FE_LIB_DIR "${OBS_SOURCE_DIR}/build/UI/obs-frontend-api")
endif()

find_library(OBS_LIB    NAMES obs libobs
    PATHS "${OBS_LIB_DIR}"    NO_DEFAULT_PATH REQUIRED)
find_library(OBS_FE_LIB NAMES obs-frontend-api
    PATHS "${OBS_FE_LIB_DIR}" NO_DEFAULT_PATH REQUIRED)

message(STATUS "OBS include  : ${OBS_INCLUDE_DIR}")
message(STATUS "OBS FE include: ${OBS_FE_INCLUDE_DIR}")
message(STATUS "OBS lib      : ${OBS_LIB}")
message(STATUS "OBS FE lib   : ${OBS_FE_LIB}")

# ── Sources ───────────────────────────────────────────────────
set(SOURCES
    src/plugin-main.cpp
    src/confidence-monitor.cpp
    src/confidence-monitor-dock.cpp
)
set(HEADERS  src/confidence-monitor.hpp)
set(RESOURCES src/resources.qrc)

# ── Target ───────────────────────────────────────────────────
add_library(confidence-monitor MODULE ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(confidence-monitor PRIVATE
    ${OBS_INCLUDE_DIR}
    ${OBS_FE_INCLUDE_DIR}
    src
)

target_link_libraries(confidence-monitor PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    ${OBS_LIB}
    ${OBS_FE_LIB}
)

set_target_properties(confidence-monitor PROPERTIES
    PREFIX ""
    OUTPUT_NAME "confidence-monitor"
)

# ── Install ───────────────────────────────────────────────────
set(OBS_PLUGIN_DIR "$ENV{APPDATA}/obs-studio/plugins" CACHE PATH
    "OBS plugin install directory")

install(TARGETS confidence-monitor
    LIBRARY DESTINATION "${OBS_PLUGIN_DIR}/confidence-monitor/bin/64bit"
    RUNTIME DESTINATION "${OBS_PLUGIN_DIR}/confidence-monitor/bin/64bit"
)
install(DIRECTORY data/
    DESTINATION "${OBS_PLUGIN_DIR}/confidence-monitor/data"
)
