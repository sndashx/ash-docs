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

# CPM.cmake — real bootstrap (downloaded on first configure).
include(cmake/CPM.cmake)

# Phase 00 third-party dependencies (spec step 0002).
# CPM 0.38.7 auto-prepends 'v' to VERSION. Upstreams vary on whether
# tags use the v-prefix, so we pass GIT_TAG explicitly to lock the ref.
CPMAddPackage(NAME entt   GIT_TAG v3.13.0 GITHUB_REPOSITORY skypjack/entt)
CPMAddPackage(NAME spdlog GIT_TAG v1.12.0 GITHUB_REPOSITORY gabime/spdlog)
CPMAddPackage(NAME fmt    GIT_TAG 10.1.1  GITHUB_REPOSITORY fmtlib/fmt)
CPMAddPackage(NAME json   GIT_TAG v3.11.3 GITHUB_REPOSITORY nlohmann/json)

if(ASH_BUILD_TESTS)
    CPMAddPackage(NAME Catch2 GIT_TAG v3.5.3 GITHUB_REPOSITORY catchorg/Catch2)
endif()

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

# Phase 00 third-party link targets (public so consumers inherit).
if(TARGET entt::entt)
    target_link_libraries(ash_lib PUBLIC entt::entt)
endif()
if(TARGET spdlog::spdlog)
    target_link_libraries(ash_lib PUBLIC spdlog::spdlog)
endif()
if(TARGET fmt::fmt)
    target_link_libraries(ash_lib PUBLIC fmt::fmt)
endif()
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(ash_lib PUBLIC nlohmann_json::nlohmann_json)
endif()

# Executable
add_executable(ash src/main.cpp)
target_link_libraries(ash PRIVATE ash_lib)
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
