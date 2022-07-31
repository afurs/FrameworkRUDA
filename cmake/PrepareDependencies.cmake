include_guard()

# MAIN PATHS
string(REPLACE ":" ";" LIBRARY_DIRS $ENV{LD_LIBRARY_PATH})
string(REPLACE ":" ";" INCLUDE_DIRS $ENV{ROOT_INCLUDE_PATH})

# BOOST: Looking for lib path
foreach(entry ${LIBRARY_DIRS})
  string(FIND ${entry} "/boost/" BOOST_POS)
  if(NOT ${BOOST_POS} EQUAL -1)
    set(BOOST_LIBRARY_PATH "${entry}")
  endif()
  string(FIND ${entry} "/ROOT/" ROOT_POS)
  if(NOT ${ROOT_POS} EQUAL -1)
    set(ROOT_PACKAGE_LOCATION "${entry}")
    string(REPLACE "/lib" "/cmake" ROOT_PACKAGE_LOCATION ${entry})
    # set(BOOST_INCLUDE_PATH ${entry})
  endif()

endforeach()

# BOOST: Looking for include path
foreach(entry ${INCLUDE_DIRS})
  string(FIND ${entry} "/boost/" BOOST_POS)
  if(NOT ${BOOST_POS} EQUAL -1)
    set(BOOST_INCLUDE_DIRS "${entry}")
    string(REPLACE "/include" "" BOOST_ROOT_TMP ${entry})
    set(BOOST_INCLUDE_PATH ${entry})
  endif()

  string(FIND ${entry} "/ROOT/" ROOT_POS)
#    message(STATUS "CHECK2: ${entry}")
  if(NOT ${ROOT_POS} EQUAL -1)
    set(ROOT_PACKAGE_LOCATION "${entry}")
#    message(STATUS "CHECK: ${entry}")
#    string(REPLACE "/include" "/cmake" ROOT_PACKAGE_LOCATION ${entry})
    # set(BOOST_INCLUDE_PATH ${entry})
  endif()
endforeach()


# ROOT
message(STATUS "CHECK: ${ROOT_PACKAGE_LOCATION}")

find_package(ROOT CONFIG REQUIRED PATHS ${ROOT_PACKAGE_LOCATION} NO_CMAKE_SYSTEM_PATH NO_DEFAULT_PATH)
if(ROOT_FOUND)
  message(STATUS "ROOT - found")
  message(STATUS "ROOT dir: ${ROOT_DIR}")
  message(STATUS "ROOT include: ${ROOT_INCLUDE_DIRS}")
else()
  message(STATUS "ROOT - not found")
endif()

# BOOST
set( Boost_NO_SYSTEM_PATHS on CACHE BOOL "Do not search system for Boost" )
set(Boost_NO_BOOST_CMAKE TRUE)
set(BOOST_LIB_PATH)

message(STATUS "Looking for Boost: ${BOOST_ROOT_TMP}")
#message(STATUS "Looking for Boost libs: ${BOOST_LIBRARY_PATH}")
#message(STATUS "Looking for Boost includes: ${BOOST_INCLUDE_PATH}")
set(BOOST_ROOT ${BOOST_ROOT_TMP})
#set(BOOST_INCLUDEDIR ${BOOST_INCLUDE_PATH})
#set(BOOST_LIBRARYDIR ${BOOST_LIBRARY_PATH})
find_package(Boost 1.70 REQUIRED COMPONENTS filesystem)
if(Boost_FOUND)
  message(STATUS "Boost include: ${Boost_INCLUDE_DIRS}")
set(Boost_VERSION_MINOR 1.70)
  message(STATUS "Boost lib: ${Boost_LIBRARIES}")
else()
  message(STATUS "Boost - not found")
endif()

# ALIROOT
find_package(AliRoot)
if(AliRoot_FOUND)
  message(STATUS "AliROOT - found")
  message(STATUS "AliROOT dir: ${AliRoot_DIR}")
  message(STATUS "AliROOT include: ${AliRoot_INCLUDE_DIRS}")
else()
  message(STATUS "AliROOT - not found")
endif()
