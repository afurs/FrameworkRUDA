# Module
set(MODULE CommonRUDA)

# Sources in alphabetical order
set(SRCS
    src/AnalysisUtils.cxx
    src/DynamicTable.cxx
    src/FunctionTraits.cxx
    src/HelperCommon.cxx
    src/HelperHists.cxx
    src/HistUtils.cxx
    src/InputFileManager.cxx
    src/OutputHistManager.cxx
    src/OutputManager.cxx
    src/Trends.cxx
    )


# Headers from sources
string(REPLACE ".cxx" ".h" HDRS "${SRCS}")
string(REPLACE "src" "include/${MODULE}" HDRS "${HDRS}")

add_library(${MODULE}-object OBJECT ${SRCS} )
target_include_directories(${MODULE}-object PRIVATE ${ROOT_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${MODULE}-object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${MODULE}-object PUBLIC ${Boost_FILESYSTEM_LIBRARY})


add_library(${MODULE} SHARED $<TARGET_OBJECTS:${MODULE}-object>)
target_link_libraries(${MODULE} PUBLIC ROOT::RIO ROOT::Hist ROOT::Core ROOT::Tree)
#target_link_libraries(${MODULE} INTERFACE ${Boost_FILESYSTEM_LIBRARY})

target_compile_options(${MODULE} PUBLIC -fPIC)

set_target_properties(${MODULE} PROPERTIES PUBLIC_HEADER "${HDRS}")

install(TARGETS ${MODULE}
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/${MODULE})


