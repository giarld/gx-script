if (NOT GX_SCRIPT_FIND_PATH)
    message(FATAL_ERROR "Not set: GX_SCRIPT_FIND_PATH")
endif ()

set(GX_SCRIPT_INCLUDE_DIR ${GX_SCRIPT_FIND_PATH}/include)
set(GX_SCRIPT_BIN_DIR ${GX_SCRIPT_FIND_PATH}/bin)
set(GX_SCRIPT_LIB_DIR ${GX_SCRIPT_FIND_PATH}/lib)

add_library(gx-script INTERFACE)
target_include_directories(gx-script INTERFACE ${GX_SCRIPT_INCLUDE_DIR})
target_link_libraries(gx-script INTERFACE
        ${GX_SCRIPT_LIB_DIR}/${LINK_LIBRARY_PREFIX}gx-script${LINK_LIBRARY_SUFFIX}
        gx)
