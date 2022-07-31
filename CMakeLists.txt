###############################
# cmake FrameworkRUDA
###############################

# cmake settings
cmake_minimum_required(VERSION 3.17)
project(
        FrameworkRUDA
        DESCRIPTION "Framework RUDA - ROOT Utilities for Data Analysis"
        VERSION 1.0 
        LANGUAGES CXX
)
get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

# global compilation options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
find_package(ROOT CONFIG REQUIRED)

if(ROOT_FOUND)
  message(STATUS "ROOT - found")
  message(STATUS "ROOT dir: ${ROOT_DIR}")
  message(STATUS "ROOT include: ${ROOT_INCLUDE_DIRS}")
else()
  message(STATUS "ROOT - not found")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
find_package(AliRoot)
if(AliRoot_FOUND)
  message(STATUS "AliROOT - found")
  message(STATUS "AliROOT dir: ${AliRoot_DIR}")
  message(STATUS "AliROOT include: ${AliRoot_INCLUDE_DIRS}")
else()
  message(STATUS "AliROOT - not found")
endif()


set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost REQUIRED COMPONENTS filesystem)

if(Boost_FOUND)
  message(STATUS "Boost include: ${Boost_INCLUDE_DIRS}")
set(Boost_VERSION_MINOR 1.70)
  message(STATUS "Boost lib: ${Boost_LIBRARIES}")

else()
  message(STATUS "Boost - not found")
endif()

message(STATUS "Install path: ${CMAKE_INSTALL_PREFIX}")

# define project directories

add_subdirectory(Framework)
add_subdirectory(Utilities)

install(DIRECTORY macro DESTINATION ${CMAKE_INSTALL_PREFIX})

file(WRITE ${CMAKE_INSTALL_PREFIX}/bin/env_ruda.sh
"#!/bin/bash
export RUDA_FRAMEWORK_BUILD_PATH=${CMAKE_INSTALL_PREFIX}
export RUDA_FRAMEWORK_BUILD_GRIDPATH=${PATH_TO_GRIDRUDA}
export ALICE_LOGBOOK_PATH=${PATH_TO_LOCAL_LOGBOOK}
export ALICE_LOGBOOK_GRIDPATH=${PATH_TO_GRID_LOGBOOK}
")
file(WRITE ${CMAKE_INSTALL_PREFIX}/bin/update_grid_ruda.sh
"#!/bin/bash
alien_cp file://$RUDA_FRAMEWORK_BUILD_PATH/include/* alien://$RUDA_FRAMEWORK_BUILD_GRIDPATH
alien_cp file://$RUDA_FRAMEWORK_BUILD_PATH/lib/* alien://$RUDA_FRAMEWORK_BUILD_GRIDPATH
")
file(WRITE ${CMAKE_INSTALL_PREFIX}/bin/update_grid_logbook.sh
"#!/bin/bash
alien_cp file://$ALICE_LOGBOOK_PATH/* alien://$ALICE_LOGBOOK_GRIDPATH
")
