# CPM.cmake bootstrap — STUB.
# The real bootstrap pulls from https://github.com/cpm-cmake/CPM.cmake
# Pin target: CPM_DOWNLOAD_VERSION 0.38.7
set(CPM_DOWNLOAD_VERSION 0.38.7)

# Try to fetch CPM from the upstream release. If offline, fall back to a
# no-op so that CMake configure still succeeds.
set(CPM_DOWNLOAD_URL
    "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake")

foreach(download_dir
    "${CMAKE_SOURCE_DIR}/.cache/CPM"
    "${CMAKE_BINARY_DIR}/.cache/CPM")
    if(EXISTS "${download_dir}/CPM.cmake")
        include("${download_dir}/CPM.cmake")
        return()
    endif()
endforeach()

find_package(CURL REQUIRED)
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
