cmake_minimum_required(VERSION 3.16)
project(confidence-monitor VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Qt6 ──────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)
qt_standard_project_setup()

# ── OBS SDK path — adjust if needed ──────────────────────────
# Set OBS_SOURCE_DIR to your OBS source checkout, OR
# set OBS_INCLUDE_DIR / OBS_LIB_DIR if you have a pre-built SDK.
#
# Example with obs-studio source:
#   cmake -DOBS_SOURCE_DIR="C:/obs-studio" ..
#
if(DEFINED OBS_SOURCE_DIR)
    set(OBS_INCLUDE_DIR "${OBS_SOURCE_DIR}/libobs")
    set(OBS_FRONTEND_INCLUDE "${OBS_SOURCE_DIR}/UI/obs-frontend-api")
    find_library(OBS_LIB     obs      PATHS "${OBS_SOURCE_DIR}/build/libobs" REQUIRED)
    find_library(OBS_FE_LIB  obs-frontend-api
                 PATHS "${OBS_SOURCE_DIR}/build/UI/obs-frontend-api" REQUIRED)
else()
    # Pre-built SDK layout (obs-studio installed to C:/Program Files/obs-studio)
    set(OBS_INSTALL_DIR "C:/Program Files/obs-studio" CACHE PATH "OBS install dir")
    set(OBS_INCLUDE_DIR     "${OBS_INSTALL_DIR}/include")
    set(OBS_FRONTEND_INCLUDE "${OBS_INSTALL_DIR}/include")
    find_library(OBS_LIB     obs      PATHS "${OBS_INSTALL_DIR}/bin/64bit" REQUIRED)
    find_library(OBS_FE_LIB  obs-frontend-api
                 PATHS "${OBS_INSTALL_DIR}/bin/64bit" REQUIRED)
endif()

# ── Sources ───────────────────────────────────────────────────
set(SOURCES
    src/plugin-main.cpp
    src/confidence-monitor.cpp
    src/confidence-monitor-dock.cpp
)

set(HEADERS
    src/confidence-monitor.hpp
)

set(RESOURCES
    src/resources.qrc
)

# ── Target ───────────────────────────────────────────────────
add_library(confidence-monitor MODULE ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(confidence-monitor PRIVATE
    ${OBS_INCLUDE_DIR}
    ${OBS_FRONTEND_INCLUDE}
    src
)

target_link_libraries(confidence-monitor PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    ${OBS_LIB}
    ${OBS_FE_LIB}
)

# ── Output name matches OBS convention ───────────────────────
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
