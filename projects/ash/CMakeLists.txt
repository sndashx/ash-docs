cmake_minimum_required(VERSION 3.21)
project(ash VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CompilerWarnings.cmake)

option(ASH_BUILD_TESTS            "Build tests"              ON)
option(ASH_ENABLE_ASAN            "Enable AddressSanitizer"  OFF)
option(ASH_ENABLE_UBSAN           "Enable UndefinedBehaviorSanitizer" OFF)
option(ASH_WARNINGS_AS_ERRORS     "Treat warnings as errors" OFF)
option(ASH_HOT_RELOAD_CONTENT     "Hot-reload content files" OFF)
option(ASH_PROFILE                "Enable Tracy profiler"    OFF)

# CPM.cmake stub. The user will replace cmake/CPM.cmake with a real
# CPM bootstrap; until then we no-op so configure succeeds offline.
include(cmake/CPM.cmake)

# Optional: ASan / UBSan
if(ASH_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
if(ASH_ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
endif()

# Aggregator library — collects every translation unit under src/ except
# the entry point (src/main.cpp), which lives only in the executable.
file(GLOB_RECURSE ASH_LIB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
list(REMOVE_ITEM ASH_LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(ash_lib STATIC ${ASH_LIB_SOURCES})
target_include_directories(ash_lib PUBLIC src)
target_compile_features(ash_lib PUBLIC cxx_std_20)
ash_set_project_warnings(ash_lib)

# Executable
add_executable(ash src/main.cpp)
# Use --whole-archive so the render pipeline primitives remain visible to
# `nm build/ash` even though main() does not yet reference them directly.
# Phase 1's --render-test mode will exercise these; until then this keeps
# the symbols (and any vtables) in the final binary for verification.
target_link_libraries(ash PRIVATE
    -Wl,--whole-archive ash_lib -Wl,--no-whole-archive
)
ash_set_project_warnings(ash)

if(MSVC)
    target_compile_options(ash PRIVATE /W4 /permissive-)
endif()

# Tests
if(ASH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "ASH ${PROJECT_VERSION} configured. Tests=${ASH_BUILD_TESTS}, ASan=${ASH_ENABLE_ASAN}, UBSan=${ASH_ENABLE_UBSAN}")
