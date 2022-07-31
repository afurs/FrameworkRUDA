# Module
set(MODULE AnalysisRUDA)

# Sources in alphabetical order
set(SRCS
    src/AnalysisBase.cxx
    src/AnalysisManagerBase.cxx
    src/CutObjectManager.cxx
    src/DataModel.cxx
    )


# Headers from sources
string(REPLACE ".cxx" ".h" HDRS "${SRCS}")
string(REPLACE "src" "include/${MODULE}" HDRS "${HDRS}")

string(REPLACE ";" ";${CMAKE_CURRENT_SOURCE_DIR}/" ${MODULE}_PATHHDRS_TMP "${HDRS}")
string(CONCAT ${MODULE}_PATHHDRS_TMP "${CMAKE_CURRENT_SOURCE_DIR}/" "${${MODULE}_PATHHDRS_TMP}")
set(${MODULE}_PATHHDRS ${${MODULE}_PATHHDRS_TMP} CACHE INTERNAL "")
set(${MODULE}_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")

add_library(${MODULE}-object OBJECT ${SRCS})
target_include_directories(${MODULE}-object PRIVATE ${ROOT_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} $<TARGET_PROPERTY:CommonRUDA,SOURCE_DIR> ${CMAKE_CURRENT_SOURCE_DIR}/include/${MODULE})

add_library(${MODULE} SHARED $<TARGET_OBJECTS:${MODULE}-object> $<TARGET_OBJECTS:CommonRUDA-object>)
target_include_directories(${MODULE} PRIVATE ${ROOT_INCLUDE_DIRS} $<TARGET_PROPERTY:CommonRUDA,SOURCE_DIR>)
target_link_libraries(${MODULE} PRIVATE ROOT::RIO ROOT::Hist ROOT::Core ROOT::Tree)
target_compile_options(${MODULE} PRIVATE -fPIC)

set_target_properties(${MODULE} PROPERTIES NO_SONAME OFF)
set_target_properties(${MODULE} PROPERTIES PUBLIC_HEADER "${HDRS}")

install(TARGETS ${MODULE}
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/${MODULE})


