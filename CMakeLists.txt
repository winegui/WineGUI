cmake_minimum_required(VERSION 3.25)

set(PROJECT_NAME WineGUI)
set(PROJECT_TARGET winegui)

option(DOXYGEN "Build Documentation" OFF)
option(PACKAGE "Build packages in release mode" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# If we have a git folder, get GIT_TAG_VERSION from most recent git tag
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  include(git_version)
  set(LOCAL_PROJECT_VERSION ${GIT_TAG_VERSION})
else()
  # In source archives there is no .git folder present.
  # If CUSTOM_PROJECT_VERSION is defined use it as version.
  if(DEFINED CUSTOM_PROJECT_VERSION)
      set(LOCAL_PROJECT_VERSION "${CUSTOM_PROJECT_VERSION}")
  elseif(EXISTS "${CMAKE_SOURCE_DIR}/version.txt")
    # Read version from version.txt file
    file(READ "version.txt" LOCAL_PROJECT_VERSION)
  else()
    # Fallback to version 1.0.0
    set(LOCAL_PROJECT_VERSION "1.0.0")
  endif()
endif()

# Include cmake GTK GSettings schema
include(g_settings)

project(${PROJECT_NAME}
  VERSION ${LOCAL_PROJECT_VERSION}
  DESCRIPTION "WineGUI is a user-friendly WINE graphical interface"
  LANGUAGES CXX)

message(STATUS "Project version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Create packages: ${PACKAGE}")
message(STATUS "Generate documentation: ${DOXYGEN}")

# Global CMake settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Use the package PkgConfig to detect (any version) of GTK+ headers/library files
find_package(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTKMM REQUIRED gtkmm-3.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set (DATADIR share CACHE PATH "datadir")

# Production build and packaging is ON
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release" AND PACKAGE)
  # Include CPack packaging settings
  include(packaging)
endif()

configure_file(include/project_config.h.in ${CMAKE_BINARY_DIR}/project_config.h)

set(HEADERS
  include/menu.h
  include/app_list_model_column.h
  include/app_list_struct.h
  include/main_window.h
  include/add_app_window.h
  include/remove_app_window.h
  include/preferences_window.h
  include/bottle_edit_window.h
  include/bottle_clone_window.h
  include/bottle_configure_env_var_window.h
  include/bottle_configure_window.h
  include/busy_dialog.h
  include/bottle_manager.h
  include/bottle_config_file.h
  include/bottle_item.h
  include/bottle_new_assistant.h
  include/about_dialog.h
  include/general_config_file.h
  include/helper.h
  include/signal_controller.h
)

set(SOURCES
  src/main.cc
  src/menu.cc
  src/main_window.cc
  src/add_app_window.cc
  src/remove_app_window.cc
  src/preferences_window.cc
  src/bottle_edit_window.cc
  src/bottle_clone_window.cc
  src/bottle_configure_env_var_window.cc
  src/bottle_configure_window.cc
  src/busy_dialog.cc
  src/bottle_manager.cc
  src/bottle_config_file.cc
  src/bottle_item.cc
  src/bottle_new_assistant.cc
  src/about_dialog.cc
  src/general_config_file.cc
  src/helper.cc
  src/signal_controller.cc
  ${HEADERS}
)

# Install and recompile glib gsettings schema
add_schema("org.melroy.winegui.gschema.xml" GSCHEMA_RING)

add_executable(${PROJECT_TARGET} ${GSCHEMA_RING} ${SOURCES})

# Set C++ standard to C++23 and disable C++ extensions
set_target_properties(${PROJECT_TARGET} PROPERTIES CXX_STANDARD 23)
set_target_properties(${PROJECT_TARGET} PROPERTIES CXX_EXTENSIONS OFF)

# Linking Threads and GTKMM
target_link_libraries(${PROJECT_TARGET} Threads::Threads ${CMAKE_THREAD_LIBS_INIT} ${GTKMM_LIBRARIES})

target_include_directories(${PROJECT_TARGET} PRIVATE ${GTKMM_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR})
target_link_directories(${PROJECT_TARGET} PRIVATE ${GTKMM_LIBRARY_DIRS})
target_compile_options(${PROJECT_TARGET} PRIVATE ${GTKMM_CFLAGS_OTHER})

install(TARGETS ${PROJECT_TARGET} RUNTIME DESTINATION "bin" COMPONENT applications)
install(FILES misc/winegui.desktop DESTINATION ${DATADIR}/applications)
install(FILES misc/winegui.png DESTINATION ${DATADIR}/icons/hicolor/48x48/apps)
install(FILES misc/winegui.svg DESTINATION ${DATADIR}/icons/hicolor/scalable/apps)
install(DIRECTORY images DESTINATION ${DATADIR}/winegui)
# Could be easily extended with eg. scripts:
#   DIRECTORY images scripts/ DESTINATION ....
#   PATTERN "scripts/*"

# To create 'make run'
add_custom_target( run
  COMMAND "bin/${PROJECT_TARGET}" 
  COMMENT "Starting up..." )

############
# Doxygen  #
############
if(DOXYGEN)
  include(doxygen)
endif()


