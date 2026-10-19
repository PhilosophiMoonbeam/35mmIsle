# Single Original Egg C provider; include after SnapEngine.cmake creates isle-engine.
include_guard(GLOBAL)
if(NOT TARGET isle-engine)
    message(FATAL_ERROR "CommonEggSource requires the existing isle-engine owner")
endif()
set(_isle_common_egg_manifest "${PROJECT_SOURCE_DIR}/cmake/common-egg-inputs.json")
set(_isle_common_egg_private "${CMAKE_BINARY_DIR}/_deps/isle-common-egg-private")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${_isle_common_egg_manifest}"
    "${PROJECT_SOURCE_DIR}/tools/prepare_common_egg.py"
    "${PROJECT_SOURCE_DIR}/tools/prepare_engine.py")
file(READ "${_isle_common_egg_manifest}" _isle_common_egg_authority)
string(JSON _isle_common_egg_support_count LENGTH "${_isle_common_egg_authority}" native_support)
math(EXPR _isle_common_egg_support_last "${_isle_common_egg_support_count} - 1")
foreach(_isle_common_egg_support_index RANGE 0 ${_isle_common_egg_support_last})
    string(JSON _isle_common_egg_support_path MEMBER "${_isle_common_egg_authority}"
        native_support ${_isle_common_egg_support_index})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/${_isle_common_egg_support_path}")
endforeach()
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -B "${PROJECT_SOURCE_DIR}/tools/prepare_common_egg.py"
        --repository "${_isle_snap_source}" --manifest "${_isle_common_egg_manifest}"
        --output "${_isle_common_egg_private}/game.c"
        --evidence "${_isle_common_egg_private}/evidence.json"
    RESULT_VARIABLE _isle_common_egg_result
    OUTPUT_VARIABLE _isle_common_egg_output
    ERROR_VARIABLE _isle_common_egg_error)
if(NOT _isle_common_egg_result EQUAL 0)
    message(FATAL_ERROR "Private common Original Egg preparation failed:\n${_isle_common_egg_output}${_isle_common_egg_error}")
endif()
# Both course actor objects import these externs. Never add the generated TU to
# either course target: isle-engine is the sole compiled owner of all six.
target_sources(isle-engine PRIVATE
    "${PROJECT_SOURCE_DIR}/src/egg_source.c"
    "${PROJECT_SOURCE_DIR}/src/egg_source.h"
    "${_isle_common_egg_private}/game.c")
set_property(SOURCE "${_isle_common_egg_private}/game.c" APPEND PROPERTY COMPILE_OPTIONS
    "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-frounding-math;-ffp-contract=off;-fno-fast-math>"
    "$<$<COMPILE_LANG_AND_ID:C,GNU>:-fexcess-precision=standard>"
    "$<$<COMPILE_LANG_AND_ID:C,MSVC>:/fp:strict>")
