cmake_minimum_required(VERSION 3.20)

project(GxScript)

set(TARGET_NAME gx-script)

file(GLOB_RECURSE GX_SCRIPT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
file(GLOB_RECURSE GX_SCRIPT_HDR ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

add_library(${TARGET_NAME}
        ${GX_SCRIPT_SRC}
        ${GX_SCRIPT_HDR}
)

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${TARGET_NAME} PUBLIC gany gx)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER Gx)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(${TARGET_NAME} PRIVATE BUILD_SHARED_LIBS=1)
endif ()

if (GX_SCRIPT_INLINE_LUA OR NOT BUILD_SHARED_LIBS)
    target_link_libraries(${TARGET_NAME} PRIVATE lua-static)
else ()
    target_link_libraries(${TARGET_NAME} PRIVATE lua)
endif ()

target_link_libraries(${TARGET_NAME} PRIVATE qjs)

target_compile_options(${TARGET_NAME}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<BOOL:${GNU_BIG_OBJ_FLAG_ENABLE}>>:-Wa,-mbig-obj>)

### Install
if (GX_LIBS_INSTALL_DIR)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/gx DESTINATION ${GX_LIBS_INSTALL_DIR}/gx-script/include)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gx-script.cmake DESTINATION ${GX_LIBS_INSTALL_DIR}/gx-script)
    install(TARGETS ${TARGET_NAME}
            RUNTIME DESTINATION ${GX_LIBS_INSTALL_DIR}/gx-script/bin
            LIBRARY DESTINATION ${GX_LIBS_INSTALL_DIR}/gx-script/lib
            ARCHIVE DESTINATION ${GX_LIBS_INSTALL_DIR}/gx-script/lib
            )
    if (GX_SCRIPT_INLINE_LUA)
        install(TARGETS lua-static
                RUNTIME DESTINATION ${GX_LIBS_INSTALL_DIR}/gx-script/bin
                LIBRARY DESTINATION ${GX_LIBS_INSTALL_DIR}/gx-script/lib
                ARCHIVE DESTINATION ${GX_LIBS_INSTALL_DIR}/gx-script/lib
                )
    else ()
        install(TARGETS lua
                RUNTIME DESTINATION ${GX_LIBS_INSTALL_DIR}/gx-script/bin
                LIBRARY DESTINATION ${GX_LIBS_INSTALL_DIR}/gx-script/lib
                ARCHIVE DESTINATION ${GX_LIBS_INSTALL_DIR}/gx-script/lib
                )
    endif ()
endif ()