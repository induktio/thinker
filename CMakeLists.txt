cmake_minimum_required(VERSION 3.15)

# fail on missing -B to not add build files in source root
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "You need to specify -B where build files are saved")
endif()

project(thinker C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
enable_language(RC)
# Use GNU windres style for RC compile rule
set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff <DEFINES> <INCLUDES> <FLAGS> -i <SOURCE> -o <OBJECT>")

# ---------- Common compiler flags ----------
set(COMMON_CXX_FLAGS "-pedantic -Wall -Wextra -Wshadow -Wundef -Wuseless-cast -Wpointer-arith -Wfloat-conversion")

# Add custom 'Develop' configuration
if(CMAKE_CONFIGURATION_TYPES)
    list(APPEND CMAKE_CONFIGURATION_TYPES Develop)
    list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "" FORCE)
else()
    # Set possible values for single-config generators
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "Develop")
endif()

# ---------- Output directories (place artifacts under source-root build/<config>) ----------
set(THINKER_OUTPUT_BASE "${CMAKE_SOURCE_DIR}/build")

# ---------- Debug ----------
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_CXX_FLAGS} -g -Og -DBUILD_DEBUG")

# ---------- Release ----------
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_CXX_FLAGS} -O2 -flto -fno-rtti -fno-strict-aliasing -fno-delete-null-pointer-checks -DBUILD_REL")

# ---------- Develop ----------
set(CMAKE_CXX_FLAGS_DEVELOP "${CMAKE_CXX_FLAGS_DEVELOP} ${COMMON_CXX_FLAGS} -O2 -fno-common -fno-strict-overflow -fno-strict-aliasing -fno-delete-null-pointer-checks -fno-optimize-sibling-calls")

# ---------- Thinker lib ----------
file(GLOB THINKER_LIB_SOURCES
    "src/*.cpp"
    "src/lib/*.c"
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
    $<$<OR:$<CONFIG:Release>,$<CONFIG:Develop>>:-static -s>
)
target_compile_definitions(thinkerlib PRIVATE BUILD_DLL)
set_target_properties(thinkerlib PROPERTIES PREFIX "" OUTPUT_NAME "thinker" SUFFIX ".dll")
target_link_libraries(thinkerlib PRIVATE user32 gdi32 winmm psapi)

# ---------- Launch executable ----------
file(GLOB THINKER_LAUNCH_SOURCES
    "src/launch.*"
)
# Ensure resource file is compiled as RC
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/launch.rc PROPERTIES LANGUAGE RC)

add_executable(thinker ${THINKER_LAUNCH_SOURCES})
target_compile_options(thinker PRIVATE
    $<$<CONFIG:Release>:-DBUILD_REL>
)
target_link_options(thinker PRIVATE
    $<$<OR:$<CONFIG:Release>,$<CONFIG:Develop>>:-static -s>
)
set_target_properties(thinker PROPERTIES PREFIX "" SUFFIX ".exe")
target_link_libraries(thinker PRIVATE thinkerlib user32)
