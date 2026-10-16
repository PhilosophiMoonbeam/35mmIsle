# Exact game definitions are materialized privately; no SDK build is invoked.
find_package(Git REQUIRED)
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/snap-engine-inputs.json"
    "${PROJECT_SOURCE_DIR}/tools/prepare_engine.py"
    "${PROJECT_SOURCE_DIR}/src/engine.c"
    "${PROJECT_SOURCE_DIR}/src/engine_math.h"
    "${PROJECT_SOURCE_DIR}/src/engine_internal.h"
    "${PROJECT_SOURCE_DIR}/include/35mmisle/engine.h")
file(READ "${CMAKE_CURRENT_LIST_DIR}/snap-engine-inputs.json" _isle_engine_manifest)
string(JSON _isle_snap_revision GET "${_isle_engine_manifest}" revision)
string(JSON _isle_snap_repository GET "${_isle_engine_manifest}" repository)
set(ISLE_SNAP_SOURCE_DIR "" CACHE PATH "Read-only Git repository containing the exact authoritative Snap commit; empty fetches privately")

if(ISLE_SNAP_SOURCE_DIR STREQUAL "")
    include(FetchContent)
    if(FETCHCONTENT_SOURCE_DIR_ISLE_SNAP)
        message(FATAL_ERROR "Use ISLE_SNAP_SOURCE_DIR; do not override the private upstream acquisition")
    endif()
    FetchContent_Declare(isle_snap
        GIT_REPOSITORY "${_isle_snap_repository}"
        GIT_TAG "${_isle_snap_revision}"
        GIT_SHALLOW FALSE
        GIT_SUBMODULES ""
        # Acquisition only: never configure or execute an upstream build tree.
        SOURCE_SUBDIR cmake/35mmisle-archive-only)
    FetchContent_MakeAvailable(isle_snap)
    set(_isle_snap_source "${isle_snap_SOURCE_DIR}")
else()
    get_filename_component(_isle_snap_source "${ISLE_SNAP_SOURCE_DIR}" REALPATH)
endif()

set(_isle_engine_generated "${CMAKE_BINARY_DIR}/_deps/isle-engine-private/game.c")
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/prepare_engine.py"
        --repository "${_isle_snap_source}"
        --manifest "${CMAKE_CURRENT_LIST_DIR}/snap-engine-inputs.json"
        --output "${_isle_engine_generated}"
        --evidence "${CMAKE_BINARY_DIR}/isle-snap-engine-inputs.json"
    RESULT_VARIABLE _isle_engine_result
    OUTPUT_VARIABLE _isle_engine_output
    ERROR_VARIABLE _isle_engine_error)
if(NOT _isle_engine_result EQUAL 0)
    message(FATAL_ERROR "Private native game source preparation failed:\n${_isle_engine_output}${_isle_engine_error}")
endif()
message(STATUS "${_isle_engine_output}")

add_library(isle-engine STATIC src/engine.c src/engine_internal.h src/engine_math.h
    include/35mmisle/engine.h "${_isle_engine_generated}")
target_include_directories(isle-engine PUBLIC "${PROJECT_SOURCE_DIR}/include"
    PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(isle-engine PUBLIC isle-native isle-continuation)
if(UNIX)
    target_link_libraries(isle-engine PRIVATE m)
endif()
isle_target_options(isle-engine)
target_compile_options(isle-engine PRIVATE
    "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-frounding-math;-ffp-contract=off;-fno-fast-math>"
    "$<$<COMPILE_LANG_AND_ID:C,MSVC>:/fp:strict>")

if(BUILD_TESTING)
    add_executable(engine-tests tests/engine_tests.cpp tests/engine_fixture.c)
    target_link_libraries(engine-tests PRIVATE isle-engine)
    isle_target_options(engine-tests)
    add_test(NAME engine-tests COMMAND engine-tests)
    set_tests_properties(engine-tests PROPERTIES LABELS "synthetic;private-game-source" TIMEOUT 60)
endif()

if(ISLE_BUILD_RSP_PROBE AND ISLE_BUILD_RDP_PROBE)
    # Explicit private-ROM diagnostic, deliberately not an automatic CTest.
    # Canonical executor/fixture targets are declared later by the root build.
    add_executable(engine-camera-rsp-smoke tests/engine_camera_rsp_smoke.cpp)
    target_link_libraries(engine-camera-rsp-smoke PRIVATE
        isle-engine isle-rom-snapshot isle-rsp-fixture isle-canonical-executor)
    isle_target_options(engine-camera-rsp-smoke)
endif()

message(STATUS "Native OM/model/intro-fade/camera kinds 3/6/8/14, translate-only, kind-28/photo and RT/RTS placement math definitions are private development inputs, not full scene/audio readiness or a redistribution grant")
