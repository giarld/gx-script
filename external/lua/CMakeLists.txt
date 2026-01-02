cmake_minimum_required(VERSION 3.20)

project(LUA)

file(GLOB_RECURSE LUA_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
file(GLOB_RECURSE LUA_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

add_library(lua
        ${LUA_SRCS}
        ${LUA_HDRS}
)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(lua PRIVATE BUILD_SHARED_LIBS=1)
endif ()

target_include_directories(lua PUBLIC src/)

# static
add_library(lua-static STATIC
        ${LUA_SRCS}
)

target_include_directories(lua-static PUBLIC src/)

if (WIN32)
    target_compile_definitions(lua PUBLIC LUA_USE_WINDOWS=1)
    target_compile_definitions(lua-static PUBLIC LUA_USE_WINDOWS=1)
elseif (APPLE)
    if (IOS)
        target_compile_definitions(lua PUBLIC LUA_USE_IOS=1)
        target_compile_definitions(lua-static PUBLIC LUA_USE_IOS=1)
    else ()
        target_compile_definitions(lua PUBLIC LUA_USE_MACOSX=1)
        target_compile_definitions(lua-static PUBLIC LUA_USE_MACOSX=1)
    endif ()
elseif (UNIX)
    target_compile_definitions(lua PUBLIC LUA_USE_LINUX=1)
    target_compile_definitions(lua-static PUBLIC LUA_USE_LINUX=1)
endif ()

set_target_properties(lua PROPERTIES FOLDER Gx/External)
set_target_properties(lua-static PROPERTIES FOLDER Gx/External)