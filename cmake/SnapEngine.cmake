# Exact game definitions are materialized privately; no SDK build is invoked.
find_package(Git REQUIRED)
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/snap-engine-inputs.json"
    "${PROJECT_SOURCE_DIR}/tools/prepare_engine.py"
    "${PROJECT_SOURCE_DIR}/src/engine.c"
    "${PROJECT_SOURCE_DIR}/src/engine_math.h"
    "${PROJECT_SOURCE_DIR}/src/contextual_model_math.h"
    "${PROJECT_SOURCE_DIR}/src/engine_internal.h"
    "${PROJECT_SOURCE_DIR}/src/source_factory_resources.h"
    "${PROJECT_SOURCE_DIR}/src/player_source.h"
    "${PROJECT_SOURCE_DIR}/src/player_source_bridge.c"
    "${PROJECT_SOURCE_DIR}/src/player_camera_viewport.c"
    "${PROJECT_SOURCE_DIR}/src/player_camera_viewport.h"
    "${PROJECT_SOURCE_DIR}/src/world_movement_state.h"
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

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/cmake/actor-control-inputs.json"
    "${PROJECT_SOURCE_DIR}/cmake/actor-owner-inputs.json"
    "${PROJECT_SOURCE_DIR}/tools/prepare_actor_control.py"
    "${PROJECT_SOURCE_DIR}/src/actor_source.h"
    "${PROJECT_SOURCE_DIR}/src/actor_source_bridge.c"
    "${PROJECT_SOURCE_DIR}/src/actor_owner_source.h"
    "${PROJECT_SOURCE_DIR}/src/actor_source_owner_bridge.c"
    "${PROJECT_SOURCE_DIR}/src/actor_interaction_services.h"
    "${PROJECT_SOURCE_DIR}/src/beach_initial_species_source.h"
    "${PROJECT_SOURCE_DIR}/src/source_command_scope.h"
    "${PROJECT_SOURCE_DIR}/src/item_source.h"
    "${PROJECT_SOURCE_DIR}/include/35mmisle/rng.h"
    "${PROJECT_SOURCE_DIR}/src/rng.c")
set(_isle_actor_generated "${CMAKE_BINARY_DIR}/_deps/isle-actor-control-private/game.c")
set(_isle_actor_owner_generated "${CMAKE_BINARY_DIR}/_deps/isle-actor-control-private/owner.c")
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -B "${PROJECT_SOURCE_DIR}/tools/prepare_actor_control.py"
        --repository "${_isle_snap_source}"
        --manifest "${PROJECT_SOURCE_DIR}/cmake/actor-control-inputs.json"
        --output "${_isle_actor_generated}"
        --owner-output "${_isle_actor_owner_generated}"
        --evidence "${CMAKE_BINARY_DIR}/isle-actor-control-inputs.json"
    RESULT_VARIABLE _isle_actor_result
    OUTPUT_VARIABLE _isle_actor_output
    ERROR_VARIABLE _isle_actor_error)
if(NOT _isle_actor_result EQUAL 0)
    message(FATAL_ERROR "Private native actor source preparation failed:\n${_isle_actor_output}${_isle_actor_error}")
endif()
message(STATUS "${_isle_actor_output}")

# Actor/Item/Beach chooser implementations share one native owner archive:
# their real source callbacks mutually depend on those owners, not the engine.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/cmake/item-owner-inputs.json"
    "${PROJECT_SOURCE_DIR}/tools/prepare_items.py")
file(READ "${PROJECT_SOURCE_DIR}/cmake/item-owner-inputs.json" _isle_item_manifest)
string(JSON _isle_item_support_count LENGTH "${_isle_item_manifest}" native_support)
math(EXPR _isle_item_support_last "${_isle_item_support_count} - 1")
foreach(_isle_item_support_index RANGE 0 ${_isle_item_support_last})
    string(JSON _isle_item_support_path MEMBER "${_isle_item_manifest}" native_support ${_isle_item_support_index})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/${_isle_item_support_path}")
endforeach()
set(_isle_item_generated "${CMAKE_BINARY_DIR}/_deps/isle-item-owner-private/game.c")
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -B "${PROJECT_SOURCE_DIR}/tools/prepare_items.py"
        --repository "${_isle_snap_source}"
        --manifest "${PROJECT_SOURCE_DIR}/cmake/item-owner-inputs.json"
        --output "${_isle_item_generated}"
        --evidence "${CMAKE_BINARY_DIR}/isle-item-owner-inputs.json"
    RESULT_VARIABLE _isle_item_result
    OUTPUT_VARIABLE _isle_item_output
    ERROR_VARIABLE _isle_item_error)
if(NOT _isle_item_result EQUAL 0)
    message(FATAL_ERROR "Private native Item source preparation failed:\n${_isle_item_output}${_isle_item_error}")
endif()
message(STATUS "${_isle_item_output}")

add_library(isle-engine STATIC src/engine.c src/engine_internal.h src/engine_math.h
    src/contextual_model_math.h include/35mmisle/engine.h "${_isle_engine_generated}")
target_sources(isle-engine PRIVATE src/actor_source_bridge.c src/actor_source.h "${_isle_actor_generated}")
target_sources(isle-engine PRIVATE src/item_source_bind.c src/item_source.h)
target_sources(isle-engine PRIVATE src/source_factory_resources.h)
target_sources(isle-engine PRIVATE src/player_source_bridge.c src/player_source.h src/world_movement_state.h)
target_sources(isle-engine PRIVATE src/player_camera_viewport.c src/player_camera_viewport.h)
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
    target_include_directories(engine-tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
    isle_target_options(engine-tests)
    add_test(NAME engine-tests COMMAND engine-tests)
    set_tests_properties(engine-tests PROPERTIES LABELS "synthetic;private-game-source" TIMEOUT 60)

    add_executable(actor-control-tests tests/actor_control_tests.cpp tests/actor_control_fixture.c)
    target_link_libraries(actor-control-tests PRIVATE isle-engine)
    target_include_directories(actor-control-tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
    isle_target_options(actor-control-tests)
    add_test(NAME actor-control-tests COMMAND actor-control-tests)
    set_tests_properties(actor-control-tests PROPERTIES LABELS "synthetic;private-game-source" TIMEOUT 60)

    add_executable(item-source-tests tests/item_source_tests.cpp tests/item_source_fixture.c)
    target_link_libraries(item-source-tests PRIVATE isle-engine)
    target_include_directories(item-source-tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
    isle_target_options(item-source-tests)
    add_test(NAME item-source-tests COMMAND item-source-tests)
    set_tests_properties(item-source-tests PROPERTIES LABELS "synthetic;private-game-source" TIMEOUT 60)

    add_executable(player-source-tests tests/player_source_tests.cpp tests/player_source_fixture.c)
    target_link_libraries(player-source-tests PRIVATE isle-engine)
    target_include_directories(player-source-tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
    isle_target_options(player-source-tests)
    add_test(NAME player-source-tests COMMAND player-source-tests)
    set_tests_properties(player-source-tests PROPERTIES LABELS "synthetic;private-game-source" TIMEOUT 60)

    add_executable(native-factory-resources-tests
        tests/native_factory_resources_tests.cpp tests/native_factory_resources_fixture.c)
    target_link_libraries(native-factory-resources-tests PRIVATE isle-engine)
    target_include_directories(native-factory-resources-tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
    isle_target_options(native-factory-resources-tests)
    add_test(NAME native-factory-resources-tests COMMAND native-factory-resources-tests)
    set_tests_properties(native-factory-resources-tests PROPERTIES LABELS "synthetic;private-game-source" TIMEOUT 60)
endif()

if(ISLE_BUILD_RSP_PROBE AND ISLE_BUILD_RDP_PROBE)
    # Explicit private-ROM diagnostic, deliberately not an automatic CTest.
    # Canonical executor/fixture targets are declared later by the root build.
    add_executable(engine-camera-rsp-smoke tests/engine_camera_rsp_smoke.cpp)
    target_link_libraries(engine-camera-rsp-smoke PRIVATE
        isle-engine isle-rom-snapshot isle-rsp-fixture isle-canonical-executor)
    isle_target_options(engine-camera-rsp-smoke)
endif()

message(STATUS "Native OM/model/intro-fade/camera kinds 3/6/8/14, translate-only, kind-28/photo, RT/RTS placement, source54 fixed-W and source48 basis math are private development inputs, not full scene/audio readiness or a redistribution grant")
