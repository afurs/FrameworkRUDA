# Module
set(MODULE RunManagerRUDA)

# Sources in alphabetical order
set(SRCS
    src/RunManager.cxx
    )


# Headers from sources
string(REPLACE ".cxx" ".h" HDRS "${SRCS}")
string(REPLACE "src" "include/${MODULE}" HDRS "${HDRS}")

add_library(${MODULE}-object OBJECT ${SRCS})
target_include_directories(${MODULE}-object PRIVATE ${ROOT_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(${MODULE} SHARED $<TARGET_OBJECTS:${MODULE}-object>)
target_link_libraries(${MODULE} PRIVATE ROOT::RIO ROOT::Core ROOT::Tree)

target_compile_options(${MODULE} PRIVATE -fPIC)
set_target_properties(${MODULE} PROPERTIES NO_SONAME OFF)
set_target_properties(${MODULE} PROPERTIES PUBLIC_HEADER "${HDRS}")

install(TARGETS ${MODULE}
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/${MODULE})


