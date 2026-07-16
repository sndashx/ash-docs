# CPM.cmake bootstrap — STUB.
# The real bootstrap pulls from https://github.com/cpm-cmake/CPM.cmake
# Pin target: CPM_DOWNLOAD_VERSION 0.38.7
set(CPM_DOWNLOAD_VERSION 0.38.7)

# Try to fetch CPM from the upstream release. If offline (or the file
# cannot be located), fall back to a no-op stub so CMake configure still
# succeeds. Targets that genuinely depend on CPM-provided packages will
# emit their own "missing target" errors via the stub's empty interface.
#
# NOTE: We deliberately avoid `find_package(CURL REQUIRED)` here so the
# offline no-op path is reachable on systems without libcurl-dev.
# Use PROJECT_SOURCE_DIR (not CMAKE_SOURCE_DIR) so this file is safe to
# include from a subdirectory project.
set(CPM_DOWNLOAD_URL
    "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake")

foreach(download_dir
    "${PROJECT_SOURCE_DIR}/.cache/CPM"
    "${CMAKE_BINARY_DIR}/.cache/CPM")
    if(EXISTS "${download_dir}/CPM.cmake")
        include("${download_dir}/CPM.cmake")
        return()
    endif()
endforeach()

file(DOWNLOAD "${CPM_DOWNLOAD_URL}"
    "${CMAKE_BINARY_DIR}/.cache/CPM/CPM.cmake"
    STATUS CPM_DOWNLOAD_STATUS)
list(GET CPM_DOWNLOAD_STATUS 0 CPM_DOWNLOAD_RC)
if(CPM_DOWNLOAD_RC EQUAL 0)
    include("${CMAKE_BINARY_DIR}/.cache/CPM/CPM.cmake")
else()
    message(WARNING
        "CPM.cmake download failed (status ${CPM_DOWNLOAD_RC}); "
        "running with CPM stub. Configure may be incomplete if any "
        "target depends on CPM-provided packages.")
    function(CPMAddPackage)
        # no-op stub
    endfunction()
endif()
