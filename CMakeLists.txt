cmake_minimum_required(VERSION 3.16)
project(HyprGridManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)

# Include directories for headers 
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/ui)

# Find required Qt packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt6 OPTIONAL_COMPONENTS Wayland)

# Add source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/gridmanager.cpp
    src/hyprlandapi.cpp
    src/config.cpp
    src/gridcell.cpp
    src/gridpreview.cpp
)

set(HEADERS
    src/mainwindow.h
    src/gridmanager.h
    src/hyprlandapi.h
    src/config.h
    src/gridcell.h
    src/gridpreview.h
)

set(UI
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/mainwindow.ui
)

set(RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc
)

# Create executable
add_executable(hypr-grid-manager 
    ${SOURCES} 
    ${HEADERS} 
    ${UI} 
    ${RESOURCES}
)

# Link with Qt
target_link_libraries(hypr-grid-manager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

# If Wayland component is found, link it
if(Qt6Wayland_FOUND)
    target_link_libraries(hypr-grid-manager PRIVATE Qt6::Wayland)
    target_compile_definitions(hypr-grid-manager PRIVATE USE_WAYLAND)
endif()

# Installation rules
install(TARGETS hypr-grid-manager DESTINATION bin)
install(FILES resources/hypr-grid-manager.desktop DESTINATION share/applications)
install(FILES resources/icons/hypr-grid-manager.png DESTINATION share/icons/hicolor/128x128/apps)
