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
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(cmake/PrepareDependencies.cmake)
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
