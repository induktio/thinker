cmake_minimum_required(VERSION 3.15)

project(thinker C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

# ---------- Common compiler flags ----------
set(COMMON_CXX_FLAGS "-pedantic -Wall -Wextra -Wshadow -Wundef -Wuseless-cast -Wpointer-arith -Wfloat-conversion")

# ---------- Debug ----------
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_CXX_FLAGS} -g -Og -DBUILD_DEBUG")

# ---------- Release ----------
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_CXX_FLAGS} -O2 -flto -fno-rtti -fno-strict-aliasing -fno-delete-null-pointer-checks -DBUILD_REL")

# ---------- Thinker lib ----------
file(GLOB THINKER_LIB_SOURCES
    "src/*.cpp"
    "src/lib/*.c"
    "src/*.rc"
)
set_source_files_properties(src/lib/ini.c PROPERTIES LANGUAGE C)

# Remove thinker launcher files
list(REMOVE_ITEM THINKER_LIB_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/launch.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/launch.h"
)
add_library(thinkerlib SHARED ${THINKER_LIB_SOURCES})
target_compile_options(thinkerlib PRIVATE
    -march=pentium4
    -mtune=generic
)
target_link_options(thinkerlib PRIVATE
    -static
)
target_compile_definitions(thinkerlib PRIVATE BUILD_DLL)
set_target_properties(thinkerlib PROPERTIES PREFIX "" OUTPUT_NAME "thinker" SUFFIX ".dll")
target_link_libraries(thinkerlib PRIVATE user32 gdi32 winmm psapi)

# ---------- Launch executable ----------
file(GLOB THINKER_LAUNCH_SOURCES
    "src/launch.*"
)

add_executable(thinker ${THINKER_LAUNCH_SOURCES})
target_compile_options(thinker PRIVATE
    $<$<CONFIG:Release>:-DBUILD_REL>
)
target_link_options(thinker PRIVATE
    -static
)
set_target_properties(thinker PROPERTIES PREFIX "" SUFFIX ".exe")
target_link_libraries(thinker PRIVATE thinkerlib user32)
