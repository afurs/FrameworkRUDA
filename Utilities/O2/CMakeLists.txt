# Module
set(MODULE O2_RUDA)

# Only headers in alphabetical order
set(HDRS
    include/${MODULE}/BCschemaUtils.h
    include/${MODULE}/CCDB.h
    include/${MODULE}/Constants.h
    include/${MODULE}/DetectorFIT.h
    include/${MODULE}/EventChDataParams.h
    include/${MODULE}/FunctorField.h

    include/${MODULE}/MCP.h
    include/${MODULE}/SyncDigits.h
    include/${MODULE}/TimestampInfo.h
    include/${MODULE}/Triggers.h
    include/${MODULE}/WorkerManager.h
    )
add_library(${MODULE} INTERFACE)
target_include_directories(${MODULE} INTERFACE 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(${MODULE} PROPERTIES PUBLIC_HEADER "${HDRS}")

install(TARGETS ${MODULE}
        PUBLIC_HEADER DESTINATION include/${MODULE})


