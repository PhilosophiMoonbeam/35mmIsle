# Exact MIT-selected RSP helpers and the source-backed LLVM ORC backend.
include_guard(GLOBAL)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Windows)$")
    message(FATAL_ERROR "The RSP backend supports Linux x64 and Windows MSVC x64 only")
endif()
if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR "The Windows RSP backend requires MSVC x64")
endif()
if(WIN32)
    string(TOLOWER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" _isle_rsp_arch)
    if(NOT _isle_rsp_arch STREQUAL "x64")
        message(FATAL_ERROR "The RSP backend requires an x64 compiler target")
    endif()
else()
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _isle_rsp_arch)
    if(NOT _isle_rsp_arch MATCHES "^(x86_64|amd64|x64)$")
        message(FATAL_ERROR "The RSP backend requires an x64 compiler target")
    endif()
endif()
foreach(_override IN ITEMS ISLE_RSP_SOURCE_DIR PARALLEL_RSP_SOURCE_DIR
        FETCHCONTENT_SOURCE_DIR_PARALLEL_RSP FETCHCONTENT_SOURCE_DIR_ISLE_PARALLEL_RSP)
    if(DEFINED ${_override} AND NOT "${${_override}}" STREQUAL "")
        message(FATAL_ERROR "Arbitrary RSP sources are forbidden: ${_override}")
    endif()
endforeach()

set(_isle_rsp_project "${CMAKE_CURRENT_LIST_DIR}/..")
set(_isle_rsp_private "${CMAKE_CURRENT_BINARY_DIR}/private-rsp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/rsp-dependencies.json"
    "${_isle_rsp_project}/tools/prepare_rsp.py"
    "${_isle_rsp_project}/tools/prepare_rdp.py")
execute_process(COMMAND "${Python3_EXECUTABLE}" -B "${_isle_rsp_project}/tools/prepare_rsp.py"
    --manifest "${CMAKE_CURRENT_LIST_DIR}/rsp-dependencies.json"
    --output "${_isle_rsp_private}"
    RESULT_VARIABLE _isle_rsp_prepare_result
    OUTPUT_VARIABLE _isle_rsp_prepare_output
    ERROR_VARIABLE _isle_rsp_prepare_error)
if(NOT _isle_rsp_prepare_result EQUAL 0)
    message(FATAL_ERROR "Exact RSP helper preparation failed:\n${_isle_rsp_prepare_output}\n${_isle_rsp_prepare_error}")
endif()
message(STATUS "${_isle_rsp_prepare_output}")
set(ISLE_RSP_SOURCE_ROOT "${_isle_rsp_private}/source")
set(ISLE_RSP_PROVENANCE "${_isle_rsp_private}/provenance.json")
set(ISLE_RSP_NOTICES "${_isle_rsp_private}/notices")

# Do not add_subdirectory upstream: its runners, legacy debug LLVM/Clang CPU
# and GNU Lightning backend are not part of this production composition.
include("${CMAKE_CURRENT_LIST_DIR}/LLVM.cmake")

add_library(isle-parallel-rsp STATIC
    "${ISLE_RSP_SOURCE_ROOT}/rsp/vfunctions.cpp"
    "${ISLE_RSP_SOURCE_ROOT}/rsp/ls.cpp"
    "${ISLE_RSP_SOURCE_ROOT}/rsp/reciprocal.cpp"
    "${ISLE_RSP_SOURCE_ROOT}/rsp/cp2.cpp"
    "${ISLE_RSP_SOURCE_ROOT}/arch/simd/rsp/rsp_core.cpp"
    "${_isle_rsp_project}/src/rsp_cp0.cpp")
target_include_directories(isle-parallel-rsp SYSTEM PUBLIC
    "${ISLE_RSP_SOURCE_ROOT}" "${ISLE_RSP_SOURCE_ROOT}/arch/simd/rsp")
set_target_properties(isle-parallel-rsp PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
if(MSVC)
    target_compile_definitions(isle-parallel-rsp PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(isle-parallel-rsp PRIVATE /permissive- /wd4267 /wd4244 /wd4309)
endif()
add_library(isle-rsp-llvm STATIC
    "${_isle_rsp_project}/src/rsp_llvm.cpp"
    "${_isle_rsp_project}/src/rsp_llvm.hpp")
target_include_directories(isle-rsp-llvm PUBLIC "${_isle_rsp_project}/src")
target_link_libraries(isle-rsp-llvm PUBLIC isle-parallel-rsp PRIVATE isle-llvm)
target_compile_features(isle-rsp-llvm PRIVATE cxx_std_20)


add_library(isle-rsp STATIC "${_isle_rsp_project}/src/rsp.cpp")
target_include_directories(isle-rsp PUBLIC "${_isle_rsp_project}/include")
target_link_libraries(isle-rsp PRIVATE isle-rsp-llvm)
target_compile_features(isle-rsp PUBLIC cxx_std_20)
