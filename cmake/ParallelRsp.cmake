# Private exact-source experiment. GNU lightning remains an LGPL dependency.
include_guard(GLOBAL)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Windows)$")
    message(FATAL_ERROR "The private RSP experiment supports Linux x64 and Windows MSVC x64 only")
endif()
if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR "The private Windows RSP experiment requires MSVC x64")
endif()
if(WIN32)
    string(TOLOWER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" _isle_rsp_arch)
    if(NOT _isle_rsp_arch STREQUAL "x64")
        message(FATAL_ERROR "The private RSP experiment requires an x64 compiler target")
    endif()
else()
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _isle_rsp_arch)
    if(NOT _isle_rsp_arch MATCHES "^(x86_64|amd64|x64)$")
        message(FATAL_ERROR "The private RSP experiment requires an x64 compiler target")
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
    message(FATAL_ERROR "Exact private RSP preparation failed:\n${_isle_rsp_prepare_output}\n${_isle_rsp_prepare_error}")
endif()
message(STATUS "${_isle_rsp_prepare_output}")
set(ISLE_RSP_SOURCE_ROOT "${_isle_rsp_private}/source")
set(ISLE_RSP_PROVENANCE "${_isle_rsp_private}/provenance.json")
set(ISLE_RSP_NOTICES "${_isle_rsp_private}/notices")

# Do not add_subdirectory upstream: its CMake unconditionally adds runner/fuzzer
# and DEBUG_JIT. This explicit subset links the real SIMD core and real JIT.
add_library(isle-rsp-lightning STATIC
    "${ISLE_RSP_SOURCE_ROOT}/lightning/lib/jit_disasm.c"
    "${ISLE_RSP_SOURCE_ROOT}/lightning/lib/jit_memory.c"
    "${ISLE_RSP_SOURCE_ROOT}/lightning/lib/jit_names.c"
    "${ISLE_RSP_SOURCE_ROOT}/lightning/lib/jit_note.c"
    "${ISLE_RSP_SOURCE_ROOT}/lightning/lib/jit_print.c"
    "${ISLE_RSP_SOURCE_ROOT}/lightning/lib/jit_size.c"
    "${ISLE_RSP_SOURCE_ROOT}/lightning/lib/lightning.c")
target_include_directories(isle-rsp-lightning SYSTEM PUBLIC "${ISLE_RSP_SOURCE_ROOT}/lightning/include")
target_compile_definitions(isle-rsp-lightning PUBLIC HAVE_MMAP=1)
target_link_libraries(isle-rsp-lightning PUBLIC Threads::Threads)
set_target_properties(isle-rsp-lightning PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(WIN32)
    target_sources(isle-rsp-lightning PRIVATE "${ISLE_RSP_SOURCE_ROOT}/win32/mman/sys/mman.c")
    target_include_directories(isle-rsp-lightning PRIVATE "${ISLE_RSP_SOURCE_ROOT}/win32/mman")
    target_compile_definitions(isle-rsp-lightning PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0600)
    target_compile_definitions(isle-rsp-lightning PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Architecture dispatch in private backend headers precedes lightning.h in
    # some translation units. This does not enable GNU asm: it is privately
    # adapted to __cpuidex and native Windows synchronization above.
    target_compile_definitions(isle-rsp-lightning PUBLIC __x86_64__=1)
endif()

add_library(isle-parallel-rsp STATIC
    "${ISLE_RSP_SOURCE_ROOT}/rsp/vfunctions.cpp"
    "${ISLE_RSP_SOURCE_ROOT}/rsp_jit.cpp"
    "${ISLE_RSP_SOURCE_ROOT}/jit_allocator.cpp"
    "${ISLE_RSP_SOURCE_ROOT}/rsp_disasm.cpp"
    "${ISLE_RSP_SOURCE_ROOT}/rsp/ls.cpp"
    "${ISLE_RSP_SOURCE_ROOT}/rsp/reciprocal.cpp"
    "${ISLE_RSP_SOURCE_ROOT}/rsp/cp2.cpp"
    "${ISLE_RSP_SOURCE_ROOT}/arch/simd/rsp/rsp_core.cpp"
    "${_isle_rsp_project}/src/rsp_cp0.cpp")
target_include_directories(isle-parallel-rsp SYSTEM PUBLIC
    "${ISLE_RSP_SOURCE_ROOT}" "${ISLE_RSP_SOURCE_ROOT}/arch/simd/rsp")
target_compile_definitions(isle-parallel-rsp PUBLIC PARALLEL_RSP_LIGHTNING)
target_link_libraries(isle-parallel-rsp PUBLIC isle-rsp-lightning PRIVATE ${CMAKE_DL_LIBS})
set_target_properties(isle-parallel-rsp PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
if(MSVC)
    target_compile_definitions(isle-parallel-rsp PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(isle-parallel-rsp PRIVATE /permissive- /wd4267 /wd4244 /wd4309)
endif()

add_library(isle-rsp STATIC "${_isle_rsp_project}/src/rsp.cpp")
target_include_directories(isle-rsp PUBLIC "${_isle_rsp_project}/include")
target_link_libraries(isle-rsp PRIVATE isle-parallel-rsp)
target_compile_features(isle-rsp PUBLIC cxx_std_20)
