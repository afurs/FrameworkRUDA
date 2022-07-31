# Module
set(MODULE GridRUDA)

# Sources in alphabetical order
set(SRCS
    JobObject.cxx
    GridHandlerManager.cxx
    GridManager.cxx
    GridUtils.cxx
    )


# Headers from sources
string(REPLACE ".cxx" ".h" HDRS "${SRCS}")
set(SRCS_AND_HDRS ${SRCS} ${HDRS})

string(REPLACE ";" ";${CMAKE_CURRENT_SOURCE_DIR}/" ${MODULE}_PATHHDRS_TMP "${HDRS}")
string(CONCAT ${MODULE}_PATHHDRS_TMP "${CMAKE_CURRENT_SOURCE_DIR}/" "${${MODULE}_PATHHDRS_TMP}")
set(${MODULE}_PATHHDRS ${${MODULE}_PATHHDRS_TMP} CACHE INTERNAL "")
set(${MODULE}_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")
set(${MODULE}_PATHLINKDEF "${CMAKE_CURRENT_SOURCE_DIR}/LinkDef.h" CACHE INTERNAL "")

#set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--export-all-symbols")
#root_generate_dictionary(G__${MODULE} ${HDRS} LINKDEF LinkDef.h)
#add_library(${MODULE}-object OBJECT ${SRCS} G__${MODULE}.cxx)

add_library(${MODULE}-object OBJECT ${SRCS})
target_include_directories(${MODULE}-object PRIVATE ${ROOT_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${AliRoot_INCLUDE_DIRS} $<TARGET_PROPERTY:CommonRUDA,SOURCE_DIR> ${CMAKE_CURRENT_SOURCE_DIR})
add_library(${MODULE} SHARED $<TARGET_OBJECTS:${MODULE}-object> $<TARGET_OBJECTS:CommonRUDA-object>)
target_compile_options(${MODULE} PRIVATE -fPIC)
set_target_properties(${MODULE} PROPERTIES NO_SONAME OFF)
target_link_libraries(${MODULE} PRIVATE ROOT::RIO ROOT::Core ${AliRoot_LIBRARIES}/libANALYSIS.so ${AliRoot_LIBRARIES}/libANALYSISaliceBase.so)

#set_target_properties(${MODULE} PROPERTIES PUBLIC_HEADER "${HDRS}" "${SRCS}")
#set_target_properties(${MODULE} PROPERTIES PUBLIC_HEADER "${SRCS_AND_HDRS}")
set_target_properties(${MODULE} PROPERTIES PUBLIC_HEADER "${HDRS}")

#set_target_properties(${MODULE} PROPERTIES LINK_FLAGS "-Wl,--whole-archive")
install(TARGETS ${MODULE}
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include)
# Additional compilation flags
#set_target_properties(${MODULE}-object PROPERTIES COMPILE_FLAGS "-fPIC")


