cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME WineGUI)
set(PROJECT_TARGET winegui)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

option(doc "Build Documentation" OFF)

# Get VERSION from most recent git tag
include(git_version)
# Include cmake GTK GSettings schema
include(g_settings)

# Don't forget to update the about_dialog.h VERSION as well
project(${PROJECT_NAME}
  VERSION ${GIT_TAG_VERSION}
  DESCRIPTION "WineGUI is a user-friendly WINE graphical interface"
  LANGUAGES CXX)

message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message("PROJECT_VERSION = ${PROJECT_VERSION}")

# Cmake settings
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CYGWIN)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Use the package PkgConfig to detect (any version) of GTK+ headers/library files
find_package(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTKMM REQUIRED gtkmm-3.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set (DATADIR share CACHE PATH "datadir")

# Production build
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  # Include CPack packaging settings
  include(packaging)
endif()

configure_file(src/project_config.h.in ${CMAKE_BINARY_DIR}/project_config.h)

set(HEADERS
    include/menu.h
    include/main_window.h
    include/preferences_window.h
    include/bottle_edit_window.h
    include/bottle_settings_window.h
    include/busy_dialog.h
    include/bottle_manager.h
    include/bottle_item.h
    include/bottle_new_assistant.h
    include/about_dialog.h
    include/config_file.h
    include/helper.h
    include/signal_dispatcher.h
)

set(SOURCES
    src/menu.cc
    src/main_window.cc
    src/preferences_window.cc
    src/bottle_edit_window.cc
    src/bottle_settings_window.cc
    src/busy_dialog.cc
    src/bottle_manager.cc
    src/bottle_item.cc
    src/bottle_new_assistant.cc
    src/about_dialog.cc
    src/config_file.cc
    src/helper.cc
    src/signal_dispatcher.cc
    ${HEADERS}
)

# Install and recompile glib gsettings schema
add_schema("org.melroy.winegui.gschema.xml" GSCHEMA_RING)

add_executable(${PROJECT_TARGET} ${GSCHEMA_RING} src/main.cc ${SOURCES})
target_link_libraries(${PROJECT_TARGET} Threads::Threads ${CMAKE_THREAD_LIBS_INIT} ${GTKMM_LIBRARIES})

target_include_directories(${PROJECT_TARGET} PRIVATE ${GTKMM_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR})
target_link_directories(${PROJECT_TARGET} PRIVATE ${GTKMM_LIBRARY_DIRS})
target_compile_options(${PROJECT_TARGET} PRIVATE ${GTKMM_CFLAGS_OTHER})

install(TARGETS ${PROJECT_TARGET} RUNTIME DESTINATION "bin" COMPONENT applications)
install(FILES misc/winegui.desktop DESTINATION ${DATADIR}/applications)
install(FILES misc/winegui.png DESTINATION ${DATADIR}/icons/hicolor/48x48/apps)
install(FILES misc/winegui.svg DESTINATION ${DATADIR}/icons/hicolor/scalable/apps)
install(DIRECTORY images DESTINATION ${DATADIR}/winegui)
# Could be easily extened with eg. scripts:
#   DIRECTORY images scripts/ DESTINATION ....
#   PATTERN "scripts/*"

# To create 'make run'
add_custom_target( run
  COMMAND "bin/${PROJECT_TARGET}" 
  COMMENT "Starting up..." )

############
# Doxygen  #
############
if(doc)
  include(doxygen)
endif()


