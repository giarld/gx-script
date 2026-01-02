cmake_minimum_required(VERSION 3.20)
project(GX-EX)

# APP_NAME SOURCE_FILE LIBS
function(add_test_app ARG)
    add_executable(${ARGV0} ${ARGV1})
    list(LENGTH ARGV argv_len)
    set(i 2)
    set(LIB_TAR)
    while (i LESS ${argv_len})
        list(GET ARGV ${i} argv_value)
        list(APPEND LIB_TAR ${argv_value})
        math(EXPR i "${i} + 1")
    endwhile ()
    message(STATUS "${PROJECT_NAME} add_test_app APP_NAME: ${ARGV0}, SOURCE_FILE: ${ARGV1}, target_link_libraries: ${LIB_TAR}")
    target_link_libraries(${ARGV0}
            ${LIB_TAR}
            )

    target_compile_options(${ARGV0}
            PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
            $<$<AND:$<CXX_COMPILER_ID:GNU>,$<BOOL:${GNU_BIG_OBJ_FLAG_ENABLE}>>:-Wa,-mbig-obj>)

    set_target_properties(${ARGV0} PROPERTIES FOLDER Gx/Examples)
endfunction()


add_definitions(-DGX_EXAMPLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# TestLuaScript
add_test_app(TestLuaScript test_script_lua.cpp gany gx gx-script)

set(my_plug_lua_file ${CMAKE_CURRENT_SOURCE_DIR}/lua/my_plug.lua)
set(my_plug_lsc_file ${T_EXECUTABLE_OUTPUT_PATH}/my_plug.lsc)

add_custom_target(test-my-lua-plug
        COMMAND luac -o ${my_plug_lsc_file} ${my_plug_lua_file}
        WORKING_DIRECTORY ${T_EXECUTABLE_OUTPUT_PATH}
        DEPENDS luac ${my_plug_lua_file}
        COMMENT "Compile lua script ${my_plug_lua_file}"
)
set_target_properties(test-my-lua-plug PROPERTIES FOLDER Gx/Examples)

add_dependencies(TestLuaScript test-my-lua-plug)

# TestJavaScript
add_test_app(TestJavaScript test_script_js.cpp gany gx gx-script)
