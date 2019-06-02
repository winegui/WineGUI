set(PROJECT_NAME WineGUI)
project(${PROJECT_NAME})

cmake_minimum_required(VERSION 2.8.8)

set(CMAKE_CXX_FLAGS "-g -Wall")

if (CMAKE_VERSION VERSION_LESS "3.1")
  add_definitions(-std=c++14)
else()
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  if(NOT CYGWIN)
    set(CMAKE_CXX_EXTENSIONS OFF)
  endif()
endif()

# Use the package PkgConfig to detect GTK+ headers/library files
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK REQUIRED gtk+-3.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

################################
# Normal Libraries & Executables
################################

# Add GTK libs and local include
include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${GTK_INCLUDE_DIRS}
)
link_directories(${GTK_LIBRARY_DIRS})
add_definitions (${GTK_CFLAGS_OTHER})

add_library(gui_lib src/gui.cc)
add_library(helper_lib src/helper.cc)

add_executable(winegui src/main.cc)

target_link_libraries(winegui gui_lib helper_lib ${CMAKE_THREAD_LIBS_INIT} ${GTK_LIBRARIES})

# To create 'make run'
add_custom_target( run
  COMMAND "bin/winegui" 
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
