# Complete own-GAME service bodies. Materialization never configures the decomp.
foreach(_isle_player_service IN ITEMS player-ui screen-transition player-bump beach-music
        application beach-lifecycle beach-ending beach-snorlax-kangaskhan
        beach-meowth beach-eevee-chansey beach-scyther-special rainbow-actor rainbow-lifecycle)
    string(REPLACE "-" "_" _isle_player_service_id "${_isle_player_service}")
    set(_isle_player_service_manifest "${PROJECT_SOURCE_DIR}/cmake/${_isle_player_service}-inputs.json")
    set(_isle_player_service_generator "${PROJECT_SOURCE_DIR}/tools/prepare_${_isle_player_service_id}.py")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${_isle_player_service_manifest}" "${_isle_player_service_generator}")
    file(READ "${_isle_player_service_manifest}" _isle_player_service_authority)
    string(JSON _isle_player_support_type ERROR_VARIABLE _isle_player_support_error
        TYPE "${_isle_player_service_authority}" native_support)
    if(NOT _isle_player_support_error)
        string(JSON _isle_player_support_count LENGTH "${_isle_player_service_authority}" native_support)
        if(_isle_player_support_count GREATER 0)
            math(EXPR _isle_player_support_last "${_isle_player_support_count} - 1")
            foreach(_isle_player_support_index RANGE 0 ${_isle_player_support_last})
                string(JSON _isle_player_support_path MEMBER "${_isle_player_service_authority}"
                    native_support ${_isle_player_support_index})
                set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                    "${PROJECT_SOURCE_DIR}/${_isle_player_support_path}")
            endforeach()
        endif()
    endif()
    set(_isle_${_isle_player_service_id}_generated
        "${CMAKE_BINARY_DIR}/_deps/isle-${_isle_player_service}-private/game.c")
    set(_isle_player_service_evidence
        "${CMAKE_BINARY_DIR}/isle-${_isle_player_service}-inputs.json")
    if(_isle_player_service STREQUAL "application")
        set(_isle_${_isle_player_service_id}_generated
            "${CMAKE_BINARY_DIR}/_deps/isle-application-private/application_original.inc")
    endif()
    if(_isle_player_service STREQUAL "application" OR
            _isle_player_service STREQUAL "beach-lifecycle" OR
            _isle_player_service STREQUAL "beach-ending" OR
            _isle_player_service STREQUAL "beach-snorlax-kangaskhan" OR
            _isle_player_service STREQUAL "beach-meowth" OR
            _isle_player_service STREQUAL "beach-eevee-chansey" OR
            _isle_player_service STREQUAL "beach-scyther-special" OR
            _isle_player_service STREQUAL "rainbow-actor" OR
            _isle_player_service STREQUAL "rainbow-lifecycle")
        set(_isle_player_service_evidence
            "${CMAKE_BINARY_DIR}/_deps/isle-${_isle_player_service}-private/evidence.json")
    endif()
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -B "${_isle_player_service_generator}"
            --repository "${_isle_snap_source}"
            --manifest "${_isle_player_service_manifest}"
            --output "${_isle_${_isle_player_service_id}_generated}"
            --evidence "${_isle_player_service_evidence}"
        RESULT_VARIABLE _isle_player_service_result
        OUTPUT_VARIABLE _isle_player_service_output
        ERROR_VARIABLE _isle_player_service_error)
    if(NOT _isle_player_service_result EQUAL 0)
        message(FATAL_ERROR "Private ${_isle_player_service} preparation failed:\n${_isle_player_service_output}${_isle_player_service_error}")
    endif()
endforeach()

set(_isle_player_photo_private "${CMAKE_BINARY_DIR}/_deps/isle-player-photo-private")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/tools/prepare_player_photo.py"
    "${PROJECT_SOURCE_DIR}/cmake/player-photo-inputs.json")
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -B "${PROJECT_SOURCE_DIR}/tools/prepare_player_photo.py"
        --source-repo "${_isle_snap_source}" --output "${_isle_player_photo_private}"
    RESULT_VARIABLE _isle_player_photo_result
    OUTPUT_VARIABLE _isle_player_photo_output
    ERROR_VARIABLE _isle_player_photo_error)
if(NOT _isle_player_photo_result EQUAL 0)
    message(FATAL_ERROR "Private photo source preparation failed:\n${_isle_player_photo_output}${_isle_player_photo_error}")
endif()

set(_isle_player_cart_private "${CMAKE_BINARY_DIR}/_deps/isle-player-cart-private")
set(_isle_player_cart_manifest "${PROJECT_SOURCE_DIR}/cmake/player-cart-owner-inputs.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/tools/prepare_player_cart.py"
    "${_isle_player_cart_manifest}")
file(READ "${_isle_player_cart_manifest}" _isle_player_cart_authority)
string(JSON _isle_player_cart_support_count LENGTH "${_isle_player_cart_authority}" native_support)
if(_isle_player_cart_support_count GREATER 0)
    math(EXPR _isle_player_cart_support_last "${_isle_player_cart_support_count} - 1")
    foreach(_isle_player_cart_support_index RANGE 0 ${_isle_player_cart_support_last})
        string(JSON _isle_player_cart_support_path MEMBER "${_isle_player_cart_authority}"
            native_support ${_isle_player_cart_support_index})
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            "${PROJECT_SOURCE_DIR}/${_isle_player_cart_support_path}")
    endforeach()
endif()
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -B "${PROJECT_SOURCE_DIR}/tools/prepare_player_cart.py"
        --repository "${_isle_snap_source}"
        --manifest "${_isle_player_cart_manifest}"
        --output "${_isle_player_cart_private}/player_cart_original.inc"
        --header "${_isle_player_cart_private}/player_cart_original.h"
        --evidence "${_isle_player_cart_private}/evidence.json"
    RESULT_VARIABLE _isle_player_cart_result
    OUTPUT_VARIABLE _isle_player_cart_output
    ERROR_VARIABLE _isle_player_cart_error)
if(NOT _isle_player_cart_result EQUAL 0)
    message(FATAL_ERROR "Private cart owner source preparation failed:\n${_isle_player_cart_output}${_isle_player_cart_error}")
endif()

# Canonical camera command ownership is materialized alongside the complete
# render component, but compiled into the C-only commands library separately.
set(_isle_player_render_private "${CMAKE_BINARY_DIR}/_deps/isle-player-render-private")
set(_isle_player_render_manifest "${PROJECT_SOURCE_DIR}/cmake/player-render-inputs.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/tools/prepare_player_render.py"
    "${PROJECT_SOURCE_DIR}/tools/prepare_engine.py"
    "${_isle_player_render_manifest}")
file(READ "${_isle_player_render_manifest}" _isle_player_render_authority)
string(JSON _isle_player_render_support_count LENGTH "${_isle_player_render_authority}" native_support)
if(_isle_player_render_support_count GREATER 0)
    math(EXPR _isle_player_render_support_last "${_isle_player_render_support_count} - 1")
    foreach(_isle_player_render_support_index RANGE 0 ${_isle_player_render_support_last})
        string(JSON _isle_player_render_support_path MEMBER "${_isle_player_render_authority}"
            native_support ${_isle_player_render_support_index})
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            "${PROJECT_SOURCE_DIR}/${_isle_player_render_support_path}")
    endforeach()
endif()
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -B "${PROJECT_SOURCE_DIR}/tools/prepare_player_render.py"
        --repository "${_isle_snap_source}"
        --manifest "${_isle_player_render_manifest}"
        --output "${_isle_player_render_private}"
    RESULT_VARIABLE _isle_player_render_result
    OUTPUT_VARIABLE _isle_player_render_output
    ERROR_VARIABLE _isle_player_render_error)
if(NOT _isle_player_render_result EQUAL 0)
    message(FATAL_ERROR "Private Player renderer source preparation failed:\n${_isle_player_render_output}${_isle_player_render_error}")
endif()
