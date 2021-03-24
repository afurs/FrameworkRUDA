# Module
set(MODULE AnalysisRUDA)

# Sources in alphabetical order
set(SRCS
    AnalysisBase.cxx
    AnalysisManagerBase.cxx
    CutObjectManager.cxx
    DataModel.cxx
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
target_include_directories(${MODULE}-object PRIVATE ${ROOT_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} $<TARGET_PROPERTY:CommonRUDA,SOURCE_DIR> ${CMAKE_CURRENT_SOURCE_DIR})

#file(GENERATE 
#    OUTPUT "tt.txt" 
#    CONTENT "$<TARGET_PROPERTY:CommonRUDA,SOURCE_DIR>\n"
#)
add_library(${MODULE} SHARED $<TARGET_OBJECTS:${MODULE}-object> $<TARGET_OBJECTS:CommonRUDA-object>)
target_include_directories(${MODULE} PRIVATE ${ROOT_INCLUDE_DIRS} $<TARGET_PROPERTY:CommonRUDA,SOURCE_DIR>)
target_link_libraries(${MODULE} PRIVATE ROOT::RIO ROOT::Hist ROOT::Core ROOT::Tree)

target_compile_options(${MODULE} PRIVATE -fPIC)
set_target_properties(${MODULE} PROPERTIES NO_SONAME OFF)
#set_target_properties(${MODULE} PROPERTIES PUBLIC_HEADER "${SRCS_AND_HDRS}")
set_target_properties(${MODULE} PROPERTIES PUBLIC_HEADER "${HDRS}")

#set_target_properties(${MODULE} PROPERTIES LINK_FLAGS "-Wl,--whole-archive")
install(TARGETS ${MODULE}
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include)
## Additional compilation flags
#set_target_properties(${MODULE}-object PROPERTIES COMPILE_FLAGS "-fPIC")


