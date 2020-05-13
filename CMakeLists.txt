if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()
set(PROJECT_NAME WineGUI)
set(PROJECT_TARGET winegui)
# Don't forget to update the about_dialog.h VERSION as well
project(${PROJECT_NAME} VERSION 1.6.0 DESCRIPTION "WineGUI is a user-friendly WINE graphical interface")

message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message("PROJECT_VERSION = ${PROJECT_VERSION}")

cmake_minimum_required(VERSION 2.8.0)

# Cmake settings
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CYGWIN)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
FIND_PACKAGE(Threads REQUIRED)

# Use the package PkgConfig to detect (any version) of GTK+ headers/library files
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTKMM REQUIRED gtkmm-3.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Production build
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  add_definitions(-DPRODUCTION=1)

  # Include CPack packaging settings
  include(packaging.cmake)
endif()

################################
# Normal Libraries & Executables
################################

# Add GTK libs and local include
include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${GTKMM_INCLUDE_DIRS}
)
link_directories(
  ${GTKMM_LIBRARY_DIRS}
)
add_definitions(${GTKMM_CFLAGS_OTHER})

add_library(menu_lib src/menu.cc)
add_library(windows_lib src/main_window.cc src/preferences_window.cc src/edit_window.cc src/settings_window.cc src/busy_dialog.cc)
add_library(bottle_manager_lib src/bottle_manager.cc)
add_library(bottle_item src/bottle_item.cc)
add_library(new_bottle_assis_lib src/new_bottle_assistant.cc)
add_library(about_dialog_lib src/about_dialog.cc)
add_library(helper_lib src/helper.cc)
add_library(signal_dispatcher_lib src/signal_dispatcher.cc)

add_executable(${PROJECT_TARGET} src/main.cc)

target_link_libraries(${PROJECT_TARGET} windows_lib menu_lib bottle_manager_lib bottle_item new_bottle_assis_lib about_dialog_lib helper_lib signal_dispatcher_lib Threads::Threads ${CMAKE_THREAD_LIBS_INIT} ${GTKMM_LIBRARIES})

install(TARGETS ${PROJECT_TARGET} RUNTIME DESTINATION "bin" COMPONENT applications)
install(FILES misc/winegui.desktop DESTINATION share/applications)
install(FILES misc/winegui.png DESTINATION share/icons/hicolor/48x48/apps)
install(FILES misc/winegui.svg DESTINATION share/icons/hicolor/scalable/apps)
install(DIRECTORY images DESTINATION share/winegui)
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
  find_package(Doxygen REQUIRED)
  set(DOXYGEN_CONFIG ${CMAKE_CURRENT_BINARY_DIR}/../doxygen.conf)
  
  # note the option ALL which allows to build the docs together with the application
  add_custom_target( docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM )
endif()
