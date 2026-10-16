cmake_minimum_required(VERSION 3.24)

# Verify the executable and registry checkout before project() can execute a
# vcpkg toolchain. Standalone builds retain their dependency-free entry path.
if(CMAKE_HOST_WIN32 AND (ISLE_BUILD_HOST_PROBE OR ISLE_BUILD_AUDIO_PROBE))
    if("$ENV{ISLE_WINDOWS_VCPKG_ROOT}" STREQUAL "" OR NOT CMAKE_TOOLCHAIN_FILE)
        message(FATAL_ERROR "Run tools/prepare_windows_host.ps1, then use the windows-host-release preset")
    endif()
    file(REAL_PATH "$ENV{ISLE_WINDOWS_VCPKG_ROOT}" _isle_windows_vcpkg)
    file(REAL_PATH "${CMAKE_TOOLCHAIN_FILE}" _isle_windows_toolchain
        BASE_DIRECTORY "${CMAKE_SOURCE_DIR}")
    file(REAL_PATH "${VCPKG_MANIFEST_DIR}" _isle_windows_manifest
        BASE_DIRECTORY "${CMAKE_SOURCE_DIR}")
    if(NOT _isle_windows_toolchain STREQUAL "${_isle_windows_vcpkg}/scripts/buildsystems/vcpkg.cmake"
       OR NOT _isle_windows_manifest STREQUAL "${CMAKE_SOURCE_DIR}/cmake/windows-host"
       OR NOT VCPKG_TARGET_TRIPLET STREQUAL "x64-windows-static"
       OR NOT VCPKG_HOST_TRIPLET STREQUAL "x64-windows-static"
       OR NOT VCPKG_MANIFEST_MODE OR NOT VCPKG_MANIFEST_INSTALL OR USE_AUTO_VCPKG)
        message(FATAL_ERROR "Windows host/audio builds require the frozen manifest/toolchain and static x64 triplets")
    endif()
    foreach(_isle_customization IN ITEMS VCPKG_OVERLAY_PORTS VCPKG_OVERLAY_TRIPLETS
            VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
        if(DEFINED ${_isle_customization} AND NOT "${${_isle_customization}}" STREQUAL "")
            message(FATAL_ERROR "Windows host provisioning excludes ${_isle_customization}")
        endif()
    endforeach()
    if(DEFINED CMAKE_MSVC_RUNTIME_LIBRARY AND NOT CMAKE_MSVC_RUNTIME_LIBRARY STREQUAL
            "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        message(FATAL_ERROR "Windows host/audio dependencies require the static MSVC runtime")
    endif()
    find_program(_isle_pwsh NAMES pwsh REQUIRED)
    execute_process(COMMAND "${_isle_pwsh}" -NoProfile -NonInteractive -File
        "${CMAKE_SOURCE_DIR}/tools/prepare_windows_host.ps1" -Root "${_isle_windows_vcpkg}"
        RESULT_VARIABLE _isle_windows_provisioned)
    if(NOT _isle_windows_provisioned EQUAL 0)
        message(FATAL_ERROR "Windows host source/tool verification failed before project discovery")
    endif()
    set(ENV{VCPKG_DISABLE_METRICS} 1)
    set(ENV{VCPKG_BINARY_SOURCES} clear)
endif()

project(35mmIsle VERSION 0.1.0
    DESCRIPTION "35mmIsle original native services, ROM tooling and opt-in libultraship host probe"
    LANGUAGES C CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Use an out-of-source build, for example cmake --preset linux-debug")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "35mmIsle native tooling requires a 64-bit target")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Keep the standalone MSVC tooling package independent of an installed VC runtime.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(ISLE_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(ISLE_BUILD_HOST_PROBE "Build the nonplayable real-libultraship procedural host probe" OFF)
option(ISLE_BUILD_GAME_ENGINE "Build selected authoritative game C definitions through the native private source adapter" OFF)
option(ISLE_BUILD_RDP_PROBE "Build the synthetic headless canonical RDP memory/feedback probe" OFF)
option(ISLE_BUILD_RSP_PROBE "Build bounded coprocessor-only RSP services and the private-ROM matrix probe" OFF)
option(ISLE_BUILD_AUDIO_PROBE "Build the native one-voice ROM audio probe and checked SDL3 output" OFF)
if(ISLE_ENABLE_SANITIZERS)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang)$"
       OR NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$"
       OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC"
       OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
        message(FATAL_ERROR "ISLE_ENABLE_SANITIZERS requires GCC or GNU-style Clang for both C and C++")
    endif()
endif()

function(isle_target_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANGUAGE:C,CXX>:/W4;/utf-8;/fp:strict>"
            "$<$<COMPILE_LANGUAGE:CXX>:/permissive->"
            "$<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>")
    else()
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow;-fno-fast-math;-ffp-contract=off>"
            "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow;-fno-fast-math;-ffp-contract=off>")
    endif()
    if(ISLE_ENABLE_SANITIZERS)
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANGUAGE:C,CXX>:-fsanitize=address,undefined;-fno-omit-frame-pointer;-fno-sanitize-recover=all>")
        target_link_options(${target} PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=all)
    endif()
endfunction()

add_library(isle-native STATIC src/native.c src/rng.c
    include/35mmisle/native.h include/35mmisle/rng.h)
target_include_directories(isle-native PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
isle_target_options(isle-native)
target_compile_options(isle-native PRIVATE
    "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-frounding-math>")

add_library(isle-host-clock STATIC src/host_clock.cpp src/host_clock.hpp)
isle_target_options(isle-host-clock)

add_library(isle-game-input STATIC src/game_input.cpp include/35mmisle/game_input.hpp)
target_include_directories(isle-game-input PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
isle_target_options(isle-game-input)

add_library(isle-photo-feedback STATIC src/photo_feedback.cpp include/35mmisle/photo_feedback.hpp)
target_include_directories(isle-photo-feedback PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
isle_target_options(isle-photo-feedback)

add_library(isle-photo-score STATIC src/photo_score.cpp include/35mmisle/photo_score.hpp)
target_link_libraries(isle-photo-score PUBLIC isle-photo-feedback)
isle_target_options(isle-photo-score)
target_compile_options(isle-photo-score PRIVATE
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-frounding-math>")

add_library(isle-required-work STATIC src/required_work.cpp src/required_work.hpp)
target_link_libraries(isle-required-work PUBLIC Threads::Threads)
target_include_directories(isle-required-work PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
isle_target_options(isle-required-work)

add_library(isle-rom STATIC src/rom.cpp include/35mmisle/rom.hpp)
target_include_directories(isle-rom PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
isle_target_options(isle-rom)

if(ISLE_BUILD_RSP_PROBE OR ISLE_BUILD_AUDIO_PROBE OR (ISLE_BUILD_HOST_PROBE AND ISLE_BUILD_GAME_ENGINE))
    add_library(isle-rom-snapshot STATIC src/rom_snapshot.cpp src/rom_snapshot.hpp
        src/rom_view.cpp src/rom_view.hpp)
    target_link_libraries(isle-rom-snapshot PUBLIC isle-rom)
    isle_target_options(isle-rom-snapshot)
    add_library(isle-graphics-task STATIC src/graphics_task.cpp src/graphics_task.hpp)
    target_include_directories(isle-graphics-task PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(isle-graphics-task PUBLIC isle-rom-snapshot)
    isle_target_options(isle-graphics-task)
    add_library(isle-pidgey-resources STATIC src/pidgey_resources.cpp src/pidgey_resources.hpp)
    target_include_directories(isle-pidgey-resources PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(isle-pidgey-resources PUBLIC isle-rom-snapshot)
    isle_target_options(isle-pidgey-resources)
    add_library(isle-beach-world-resources STATIC src/beach_world_resources.cpp src/beach_world_resources.hpp)
    target_include_directories(isle-beach-world-resources PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(isle-beach-world-resources PUBLIC isle-rom-snapshot)
    isle_target_options(isle-beach-world-resources)
    add_library(isle-photo-lighting STATIC src/photo_lighting.cpp src/photo_lighting.hpp)
    target_include_directories(isle-photo-lighting PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(isle-photo-lighting PUBLIC isle-rom-snapshot)
    isle_target_options(isle-photo-lighting)
endif()

add_library(isle-vpk0 STATIC src/vpk0.cpp include/35mmisle/vpk0.hpp)
target_include_directories(isle-vpk0 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
isle_target_options(isle-vpk0)

find_package(Threads REQUIRED)
add_library(isle-process STATIC src/process.cpp include/35mmisle/process.hpp)
target_include_directories(isle-process PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(isle-process PUBLIC Threads::Threads)
isle_target_options(isle-process)
target_compile_options(isle-process PRIVATE
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-frounding-math>")

add_library(isle-continuation STATIC src/continuation.cpp src/continuation_trampoline.c
    include/35mmisle/continuation.h src/continuation_trampoline.h)
target_include_directories(isle-continuation PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(isle-continuation PUBLIC isle-process)
isle_target_options(isle-continuation)

add_library(isle-sprite STATIC src/sprite.cpp include/35mmisle/sprite.hpp include/35mmisle/sprite.h)
target_include_directories(isle-sprite PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
isle_target_options(isle-sprite)

add_library(isle-sprite-producer STATIC src/sprite_producer.cpp src/sprite_producer.hpp
    src/sprite_raster.cpp src/sprite_raster.hpp)
target_include_directories(isle-sprite-producer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(isle-sprite-producer PUBLIC isle-sprite)
isle_target_options(isle-sprite-producer)

add_library(isle-audio-bank STATIC
    src/audio_bank.cpp src/audio_sequence.cpp
    include/35mmisle/audio_bank.hpp include/35mmisle/audio_sequence.hpp)
target_include_directories(isle-audio-bank PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
isle_target_options(isle-audio-bank)

add_library(isle-audio-sequence STATIC src/audio_cseq.cpp include/35mmisle/audio_cseq.hpp)
target_link_libraries(isle-audio-sequence PUBLIC isle-audio-bank)
isle_target_options(isle-audio-sequence)

add_library(isle-game-state STATIC src/game_state.cpp src/game_state.hpp)
target_include_directories(isle-game-state PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(isle-game-state PUBLIC isle-game-save isle-game-input isle-native)
isle_target_options(isle-game-state)

add_library(isle-save-image STATIC
    src/save_image.cpp src/save_hash.hpp include/35mmisle/save_image.hpp)
target_include_directories(isle-save-image PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
isle_target_options(isle-save-image)

add_library(isle-save-store STATIC src/save_store.cpp include/35mmisle/save_store.hpp)
target_link_libraries(isle-save-store PUBLIC isle-save-image)
isle_target_options(isle-save-store)

add_library(isle-game-save STATIC src/game_save.cpp include/35mmisle/game_save.hpp)
target_link_libraries(isle-game-save PUBLIC isle-save-store)
isle_target_options(isle-game-save)

add_library(isle-audio STATIC
    src/audio_catalog.cpp src/audio_synth_math.cpp src/audio_synth.cpp src/audio_effects.cpp
    src/audio_service.cpp src/audio_sfx.cpp src/audio_csp.cpp src/audio_synth_internal.hpp
    src/audio_effects_internal.hpp src/audio_dsp.hpp
    include/35mmisle/audio_synth.hpp include/35mmisle/audio_sfx.hpp
    include/35mmisle/audio_csp.hpp include/35mmisle/audio_effects.hpp)
target_link_libraries(isle-audio PUBLIC isle-audio-sequence)
isle_target_options(isle-audio)

if(TARGET isle-rom-snapshot)
    add_library(isle-game-audio STATIC src/game_audio.cpp src/game_audio.hpp)
    target_include_directories(isle-game-audio PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(isle-game-audio PUBLIC isle-audio isle-rom-snapshot)
    isle_target_options(isle-game-audio)

    add_library(isle-source-angles STATIC src/source_angles.cpp src/source_angles.hpp)
    target_link_libraries(isle-source-angles PUBLIC isle-rom-snapshot)
    isle_target_options(isle-source-angles)
    target_compile_options(isle-source-angles PRIVATE
        "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-frounding-math;-ffp-contract=off;-fno-fast-math>"
        "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/fp:strict>")

    add_library(isle-menu-resources STATIC src/menu_resources.cpp src/menu_resources.hpp)
    target_include_directories(isle-menu-resources PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(isle-menu-resources PUBLIC isle-rom-snapshot isle-sprite isle-vpk0)
    isle_target_options(isle-menu-resources)

    add_library(isle-model-assets STATIC src/model_asset_closure.cpp src/model_asset_closure.hpp
        src/source_matrix_configuration.hpp)
    target_include_directories(isle-model-assets PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(isle-model-assets PUBLIC isle-rom-snapshot)
    isle_target_options(isle-model-assets)

    add_library(isle-beach-actor-resources STATIC src/beach_actor_resources.cpp src/beach_actor_resources.hpp)
    target_link_libraries(isle-beach-actor-resources PUBLIC isle-model-assets)
    isle_target_options(isle-beach-actor-resources)

    add_library(isle-menu-animation STATIC src/menu_animation.cpp src/menu_animation.hpp
        src/menu_animation_path.cpp)
    target_link_libraries(isle-menu-animation PUBLIC isle-menu-resources)
    isle_target_options(isle-menu-animation)

    add_library(isle-menu-effects STATIC src/menu_effects.cpp src/menu_effects.hpp)
    target_link_libraries(isle-menu-effects PUBLIC isle-menu-resources isle-native)
    isle_target_options(isle-menu-effects)
endif()

# This remains only the read-only identity CLI; the host probe is a separate opt-in.
add_executable(35mmisle-rom src/rom_main.cpp)
target_link_libraries(35mmisle-rom PRIVATE isle-rom)
if(MINGW)
    # Match the CLI's Unicode wmain entry point, without creating a GUI subsystem.
    target_link_options(35mmisle-rom PRIVATE -municode)
endif()
isle_target_options(35mmisle-rom)

if(ISLE_BUILD_HOST_PROBE)
    include(cmake/Libultraship.cmake)
    add_executable(35mmisle-host-probe src/host_probe.cpp)
    target_compile_definitions(35mmisle-host-probe PRIVATE F3DEX_GBI_2
        CVAR_ENABLE_MULTI_VIEWPORTS="${CVAR_ENABLE_MULTI_VIEWPORTS}"
        ISLE_HOST_RENDERER_RESOURCES="${ISLE_HOST_RENDERER_RESOURCES}")
    target_link_libraries(35mmisle-host-probe PRIVATE libultraship Threads::Threads isle-host-clock)
    isle_target_options(35mmisle-host-probe)
endif()

include(CTest)
if(ISLE_BUILD_GAME_ENGINE)
    include(cmake/SnapEngine.cmake)
    target_sources(isle-engine PRIVATE src/menu_scene_bridge.c src/menu_scene_bridge.h)
    if(TARGET isle-pidgey-resources)
        add_library(isle-pidgey-producer STATIC src/pidgey_producer.cpp src/pidgey_producer.hpp)
        target_link_libraries(isle-pidgey-producer PUBLIC
            isle-engine isle-pidgey-resources isle-photo-lighting isle-graphics-task)
        isle_target_options(isle-pidgey-producer)
    endif()
    if(TARGET isle-beach-world-resources)
        add_library(isle-beach-world-producer STATIC
            src/beach_world_producer.cpp src/beach_world_producer.hpp)
        target_link_libraries(isle-beach-world-producer PUBLIC
            isle-engine isle-beach-world-resources isle-graphics-task)
        isle_target_options(isle-beach-world-producer)
        target_compile_options(isle-beach-world-producer PRIVATE
            "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-frounding-math;-ffp-contract=off;-fno-fast-math>"
            "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/fp:strict>")
    endif()
    if(TARGET isle-menu-resources)
        add_library(isle-menu-model-producer STATIC
            src/menu_model_producer.cpp src/menu_model_producer.hpp
            src/borrowed_model_kernel.cpp src/borrowed_model_kernel.hpp)
        target_link_libraries(isle-menu-model-producer PUBLIC
            isle-engine isle-menu-animation isle-graphics-task)
        isle_target_options(isle-menu-model-producer)

        add_library(isle-scene STATIC
            src/scene_resources.hpp src/scene_storage.cpp src/scene_storage.hpp
            src/scene_services.cpp src/scene_services.hpp)
        target_include_directories(isle-scene PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
        target_link_libraries(isle-scene PUBLIC
            isle-engine isle-sprite-producer isle-game-state isle-game-audio)
        isle_target_options(isle-scene)

        add_library(isle-environment-sound STATIC src/environment_sound.cpp src/environment_sound.hpp)
        target_link_libraries(isle-environment-sound PUBLIC isle-scene isle-source-angles)
        isle_target_options(isle-environment-sound)

        add_library(isle-menu-scene STATIC
            src/menu_title_scene.cpp src/menu_title_scene.hpp
            src/menu_ui_scene.cpp src/menu_ui_scene.hpp
            src/startup_runtime.cpp src/startup_runtime.hpp)
        target_link_libraries(isle-menu-scene PUBLIC
            isle-scene isle-menu-model-producer isle-menu-effects)
        isle_target_options(isle-menu-scene)

        add_library(isle-window STATIC
            src/window_memory.cpp src/window_memory.hpp
            src/window_resources.cpp src/window_resources.hpp
            src/window_element.cpp src/window_element.hpp
            src/window_text.cpp src/window_text.hpp
            src/window_widgets.cpp src/window_widgets.hpp src/window_widget_math.hpp
            src/window_layout.cpp)
        target_link_libraries(isle-window PUBLIC isle-scene isle-menu-resources isle-photo-lighting)
        isle_target_options(isle-window)

        add_library(isle-new-game-scene STATIC src/new_game_scene.cpp src/new_game_scene.hpp)
        target_link_libraries(isle-new-game-scene PUBLIC isle-window)
        isle_target_options(isle-new-game-scene)

        add_library(isle-oaks-lab-scene STATIC src/oaks_lab_scene.cpp src/oaks_lab_scene.hpp
            src/oaks_lab_resources.cpp src/oaks_lab_resources.hpp)
        target_link_libraries(isle-oaks-lab-scene PUBLIC isle-window)
        isle_target_options(isle-oaks-lab-scene)

        add_library(isle-world STATIC
            src/world_resources.cpp src/world_resources.hpp
            src/world_collision.cpp src/world_collision.hpp
            src/world_runtime.cpp src/world_runtime.hpp
            src/world_movement.cpp src/world_movement.hpp
            src/world_producer.cpp src/world_producer.hpp)
        target_link_libraries(isle-world PUBLIC isle-scene isle-menu-animation
            isle-beach-world-resources isle-model-assets isle-menu-model-producer isle-graphics-task)
        isle_target_options(isle-world)

        foreach(_isle_source_math_target IN ITEMS isle-scene isle-menu-scene isle-window
                isle-new-game-scene isle-oaks-lab-scene isle-world isle-environment-sound)
            target_compile_options(${_isle_source_math_target} PRIVATE
                "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-frounding-math;-ffp-contract=off;-fno-fast-math>"
                "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/fp:strict>")
        endforeach()
    endif()
    if(ISLE_BUILD_HOST_PROBE)
        target_sources(35mmisle-host-probe PRIVATE src/intro_probe.cpp src/intro_probe.h)
        target_compile_definitions(35mmisle-host-probe PRIVATE ISLE_HAS_GAME_ENGINE=1)
        target_link_libraries(35mmisle-host-probe PRIVATE isle-engine isle-sprite isle-rom-snapshot isle-vpk0)
    endif()
endif()

if(BUILD_TESTING)
    add_executable(native-tests tests/native_tests.c)
    target_link_libraries(native-tests PRIVATE isle-native)
    isle_target_options(native-tests)
    add_test(NAME native-tests COMMAND native-tests)

    add_executable(photo-feedback-tests tests/photo_feedback_tests.cpp)
    target_link_libraries(photo-feedback-tests PRIVATE isle-photo-feedback)
    isle_target_options(photo-feedback-tests)
    add_test(NAME photo-feedback-tests COMMAND photo-feedback-tests)

    add_executable(photo-score-tests tests/photo_score_tests.cpp)
    target_link_libraries(photo-score-tests PRIVATE isle-photo-score)
    isle_target_options(photo-score-tests)
    add_test(NAME photo-score-tests COMMAND photo-score-tests)

    add_executable(host-clock-tests tests/host_clock_tests.cpp)
    target_link_libraries(host-clock-tests PRIVATE isle-host-clock)
    isle_target_options(host-clock-tests)
    add_test(NAME host-clock-tests COMMAND host-clock-tests)

    add_executable(game-input-tests tests/game_input_tests.cpp)
    target_link_libraries(game-input-tests PRIVATE isle-game-input Threads::Threads)
    isle_target_options(game-input-tests)
    add_test(NAME game-input-tests COMMAND game-input-tests)

    add_executable(rom-tests tests/rom_tests.cpp)
    target_link_libraries(rom-tests PRIVATE isle-rom)
    isle_target_options(rom-tests)
    add_test(NAME rom-tests COMMAND rom-tests)

    add_executable(vpk0-tests tests/vpk0_tests.cpp)
    target_link_libraries(vpk0-tests PRIVATE isle-vpk0)
    isle_target_options(vpk0-tests)
    add_test(NAME vpk0-tests COMMAND vpk0-tests)

    add_executable(process-tests tests/process_tests.cpp)
    target_link_libraries(process-tests PRIVATE isle-process Threads::Threads)
    isle_target_options(process-tests)
    target_compile_options(process-tests PRIVATE
        "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-frounding-math>")
    add_test(NAME process-tests COMMAND process-tests)

    add_executable(message-queue-tests tests/message_queue_tests.cpp include/35mmisle/message_queue.hpp)
    target_include_directories(message-queue-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(message-queue-tests PRIVATE Threads::Threads)
    isle_target_options(message-queue-tests)
    add_test(NAME message-queue-tests COMMAND message-queue-tests)

    add_executable(continuation-tests tests/continuation_tests.cpp tests/continuation_fixture.c)
    target_link_libraries(continuation-tests PRIVATE isle-continuation)
    isle_target_options(continuation-tests)
    add_test(NAME continuation-tests COMMAND continuation-tests)

    add_executable(sprite-tests tests/sprite_tests.cpp)
    target_link_libraries(sprite-tests PRIVATE isle-sprite)
    isle_target_options(sprite-tests)
    add_test(NAME sprite-tests COMMAND sprite-tests)

    add_executable(audio-bank-tests tests/audio_bank_tests.cpp)
    target_link_libraries(audio-bank-tests PRIVATE isle-audio-bank)
    isle_target_options(audio-bank-tests)
    add_test(NAME audio-bank-tests COMMAND audio-bank-tests)

    add_executable(audio-sequence-tests tests/audio_sequence_tests.cpp)
    target_link_libraries(audio-sequence-tests PRIVATE isle-audio-bank)
    isle_target_options(audio-sequence-tests)
    add_test(NAME audio-sequence-tests COMMAND audio-sequence-tests)

    add_executable(audio-cseq-tests tests/audio_cseq_tests.cpp)
    target_link_libraries(audio-cseq-tests PRIVATE isle-audio-sequence)
    isle_target_options(audio-cseq-tests)
    add_test(NAME audio-cseq-tests COMMAND audio-cseq-tests)

    add_executable(audio-sfx-tests tests/audio_sfx_tests.cpp)
    target_link_libraries(audio-sfx-tests PRIVATE isle-audio)
    isle_target_options(audio-sfx-tests)
    add_test(NAME audio-sfx-tests COMMAND audio-sfx-tests)

    add_executable(audio-csp-tests tests/audio_csp_tests.cpp)
    target_link_libraries(audio-csp-tests PRIVATE isle-audio)
    isle_target_options(audio-csp-tests)
    add_test(NAME audio-csp-tests COMMAND audio-csp-tests)

    add_executable(audio-effects-tests tests/audio_effects_tests.cpp)
    target_link_libraries(audio-effects-tests PRIVATE isle-audio)
    isle_target_options(audio-effects-tests)
    add_test(NAME audio-effects-tests COMMAND audio-effects-tests)

    add_executable(save-image-tests tests/save_image_tests.cpp)
    target_link_libraries(save-image-tests PRIVATE isle-save-image)
    isle_target_options(save-image-tests)
    add_test(NAME save-image-tests COMMAND save-image-tests)

    add_executable(save-store-tests tests/save_store_tests.cpp)
    target_link_libraries(save-store-tests PRIVATE isle-save-store)
    isle_target_options(save-store-tests)
    add_test(NAME save-store-tests COMMAND save-store-tests)

    add_executable(game-save-tests tests/game_save_tests.cpp)
    target_link_libraries(game-save-tests PRIVATE isle-game-save)
    isle_target_options(game-save-tests)
    add_test(NAME game-save-tests COMMAND game-save-tests)

    add_executable(audio-catalog-tests tests/audio_catalog_tests.cpp)
    target_link_libraries(audio-catalog-tests PRIVATE isle-audio)
    isle_target_options(audio-catalog-tests)
    add_test(NAME audio-catalog-tests COMMAND audio-catalog-tests)

    add_executable(audio-synth-tests tests/audio_synth_tests.cpp)
    target_link_libraries(audio-synth-tests PRIVATE isle-audio)
    isle_target_options(audio-synth-tests)
    add_test(NAME audio-synth-tests COMMAND audio-synth-tests)

    add_executable(audio-service-tests tests/audio_service_tests.cpp)
    target_link_libraries(audio-service-tests PRIVATE isle-audio)
    isle_target_options(audio-service-tests)
    add_test(NAME audio-service-tests COMMAND audio-service-tests)

    add_executable(sprite-producer-tests tests/sprite_producer_tests.cpp)
    target_link_libraries(sprite-producer-tests PRIVATE isle-sprite-producer)
    isle_target_options(sprite-producer-tests)
    add_test(NAME sprite-producer-tests COMMAND sprite-producer-tests)

    if(TARGET isle-game-audio)
        add_executable(game-audio-tests tests/game_audio_tests.cpp)
        target_link_libraries(game-audio-tests PRIVATE isle-game-audio)
        isle_target_options(game-audio-tests)
        add_test(NAME game-audio-tests COMMAND game-audio-tests)
        set_tests_properties(game-audio-tests PROPERTIES LABELS "synthetic" TIMEOUT 60)
    endif()
    if(TARGET isle-model-assets)
        add_executable(model-asset-closure-tests tests/model_asset_closure_tests.cpp)
        target_link_libraries(model-asset-closure-tests PRIVATE isle-model-assets)
        isle_target_options(model-asset-closure-tests)
        add_test(NAME model-asset-closure-tests COMMAND model-asset-closure-tests)
        set_tests_properties(model-asset-closure-tests PROPERTIES LABELS "synthetic" TIMEOUT 60)
    endif()
    if(TARGET isle-beach-actor-resources)
        add_executable(beach-actor-resources-rom-smoke tests/beach_actor_resources_rom_smoke.cpp)
        target_link_libraries(beach-actor-resources-rom-smoke PRIVATE
            isle-beach-actor-resources isle-beach-world-resources isle-menu-animation)
        isle_target_options(beach-actor-resources-rom-smoke)
    endif()
    if(TARGET isle-menu-animation)
        add_executable(menu-animation-tests tests/menu_animation_tests.cpp)
        target_link_libraries(menu-animation-tests PRIVATE isle-menu-animation)
        isle_target_options(menu-animation-tests)
        add_test(NAME menu-animation-tests COMMAND menu-animation-tests)
        set_tests_properties(menu-animation-tests PROPERTIES LABELS "synthetic" TIMEOUT 60)

        add_executable(menu-animation-rom-smoke tests/menu_animation_rom_smoke.cpp)
        target_link_libraries(menu-animation-rom-smoke PRIVATE isle-menu-animation)
        isle_target_options(menu-animation-rom-smoke)
        add_executable(menu-effects-private-tests tests/menu_effects_tests.cpp)
        target_link_libraries(menu-effects-private-tests PRIVATE isle-menu-effects)
        isle_target_options(menu-effects-private-tests)
    endif()
    if(TARGET isle-menu-model-producer)
        add_executable(menu-model-tests tests/menu_model_tests.cpp)
        target_link_libraries(menu-model-tests PRIVATE isle-menu-model-producer)
        isle_target_options(menu-model-tests)
        add_test(NAME menu-model-tests COMMAND menu-model-tests)
        set_tests_properties(menu-model-tests PROPERTIES LABELS "synthetic" TIMEOUT 60)
        add_executable(menu-ui-tests tests/menu_ui_tests.cpp)
        target_link_libraries(menu-ui-tests PRIVATE isle-menu-scene)
        isle_target_options(menu-ui-tests)
        add_test(NAME menu-ui-tests COMMAND menu-ui-tests)
        add_executable(menu-scene-bridge-tests tests/menu_scene_bridge_tests.cpp)
        target_link_libraries(menu-scene-bridge-tests PRIVATE isle-engine)
        target_include_directories(menu-scene-bridge-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
        isle_target_options(menu-scene-bridge-tests)
        add_test(NAME menu-scene-bridge-tests COMMAND menu-scene-bridge-tests)
        set_tests_properties(menu-ui-tests menu-scene-bridge-tests
            PROPERTIES LABELS "synthetic" TIMEOUT 60)
    endif()
    add_executable(game-state-tests tests/game_state_tests.cpp)
    target_link_libraries(game-state-tests PRIVATE isle-game-state)
    isle_target_options(game-state-tests)
    add_test(NAME game-state-tests COMMAND game-state-tests)
    set_tests_properties(game-state-tests PROPERTIES LABELS "synthetic" TIMEOUT 60)
    if(TARGET isle-scene)
        foreach(_isle_scene_test IN ITEMS scene-storage scene-services)
            string(REPLACE "-" "_" _isle_scene_test_source "${_isle_scene_test}")
            add_executable(${_isle_scene_test}-tests tests/${_isle_scene_test_source}_tests.cpp)
            target_link_libraries(${_isle_scene_test}-tests PRIVATE isle-scene)
            isle_target_options(${_isle_scene_test}-tests)
            add_test(NAME ${_isle_scene_test}-tests COMMAND ${_isle_scene_test}-tests)
            set_tests_properties(${_isle_scene_test}-tests PROPERTIES LABELS "synthetic" TIMEOUT 60)
        endforeach()
        target_sources(scene-services-tests PRIVATE tests/engine_fixture.c)
        foreach(_isle_window_test IN ITEMS memory text element widgets)
            add_executable(window-${_isle_window_test}-tests tests/window_${_isle_window_test}_tests.cpp)
            target_link_libraries(window-${_isle_window_test}-tests PRIVATE isle-window)
            isle_target_options(window-${_isle_window_test}-tests)
            add_test(NAME window-${_isle_window_test}-tests COMMAND window-${_isle_window_test}-tests)
            set_tests_properties(window-${_isle_window_test}-tests PROPERTIES LABELS "synthetic" TIMEOUT 60)
        endforeach()
        add_executable(new-game-scene-tests tests/new_game_scene_tests.cpp)
        target_link_libraries(new-game-scene-tests PRIVATE isle-new-game-scene)
        isle_target_options(new-game-scene-tests)
        add_test(NAME new-game-scene-tests COMMAND new-game-scene-tests)
        set_tests_properties(new-game-scene-tests PROPERTIES LABELS "synthetic" TIMEOUT 60)
        foreach(_isle_world_test IN ITEMS runtime collision)
            add_executable(world-${_isle_world_test}-tests tests/world_${_isle_world_test}_tests.cpp)
            target_link_libraries(world-${_isle_world_test}-tests PRIVATE isle-world)
            isle_target_options(world-${_isle_world_test}-tests)
            add_test(NAME world-${_isle_world_test}-tests COMMAND world-${_isle_world_test}-tests)
            set_tests_properties(world-${_isle_world_test}-tests PROPERTIES LABELS "synthetic" TIMEOUT 60)
        endforeach()
        add_executable(window-resources-rom-smoke tests/window_resources_rom_smoke.cpp)
        target_link_libraries(window-resources-rom-smoke PRIVATE isle-window)
        isle_target_options(window-resources-rom-smoke)
        add_executable(world-resources-rom-smoke tests/world_resources_rom_smoke.cpp)
        target_link_libraries(world-resources-rom-smoke PRIVATE isle-world)
        isle_target_options(world-resources-rom-smoke)
        add_executable(oaks-lab-scene-tests tests/oaks_lab_scene_tests.cpp)
        target_link_libraries(oaks-lab-scene-tests PRIVATE isle-oaks-lab-scene)
        isle_target_options(oaks-lab-scene-tests)
        add_test(NAME oaks-lab-scene-tests COMMAND oaks-lab-scene-tests)
        set_tests_properties(oaks-lab-scene-tests PROPERTIES LABELS "synthetic" TIMEOUT 60)
    endif()

    # All executables use explicit failure checks, including in Release builds.
    set_tests_properties(native-tests photo-feedback-tests host-clock-tests rom-tests vpk0-tests process-tests message-queue-tests continuation-tests sprite-tests audio-bank-tests audio-sequence-tests audio-cseq-tests audio-sfx-tests audio-csp-tests audio-effects-tests audio-catalog-tests audio-synth-tests audio-service-tests save-image-tests save-store-tests
        PROPERTIES LABELS "synthetic" TIMEOUT 60)
    set_tests_properties(game-input-tests game-save-tests sprite-producer-tests
        PROPERTIES LABELS "synthetic" TIMEOUT 60)
endif()

if(ISLE_BUILD_RDP_PROBE)
    include(cmake/ParallelRdp.cmake)
    add_executable(35mmisle-rdp-probe src/rdp_probe.cpp src/rdp_fixture.cpp src/rdp_fixture.hpp)
    target_link_libraries(35mmisle-rdp-probe PRIVATE isle_parallel_rdp isle-photo-feedback)
    isle_target_options(35mmisle-rdp-probe)
    if(MINGW)
        target_link_options(35mmisle-rdp-probe PRIVATE -municode)
    endif()
endif()

if(ISLE_BUILD_RSP_PROBE OR ISLE_BUILD_AUDIO_PROBE)
    include(cmake/ParallelRsp.cmake)
    isle_target_options(isle-rsp)
    if(ISLE_ENABLE_SANITIZERS)
        foreach(_isle_rsp_dependency isle-parallel-rsp isle-rsp-lightning)
            target_compile_options(${_isle_rsp_dependency} PRIVATE
                -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
        endforeach()
    endif()
    if(ISLE_BUILD_RSP_PROBE)
        add_library(isle-rsp-packets STATIC src/rsp_packets.cpp src/rsp_packets.hpp)
        target_include_directories(isle-rsp-packets PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
        isle_target_options(isle-rsp-packets)
        add_library(isle-rsp-fixture STATIC src/rsp_fixture.cpp src/rsp_fixture.hpp)
        target_include_directories(isle-rsp-fixture PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_link_libraries(isle-rsp-fixture PUBLIC isle-rsp-packets)
        isle_target_options(isle-rsp-fixture)
        add_executable(35mmisle-rsp-probe src/rsp_probe.cpp)
        target_link_libraries(35mmisle-rsp-probe PRIVATE isle-rsp isle-rom-snapshot isle-rsp-fixture)
        isle_target_options(35mmisle-rsp-probe)
        if(ISLE_BUILD_RDP_PROBE)
            add_library(isle-canonical-executor STATIC src/rsp_render.cpp src/rsp_render.hpp)
            target_link_libraries(isle-canonical-executor PUBLIC isle-rsp isle-rsp-packets
                PRIVATE isle_parallel_rdp Threads::Threads)
            isle_target_options(isle-canonical-executor)
            if(TARGET isle-pidgey-producer)
                add_library(isle-pidgey-feedback STATIC src/pidgey_feedback.cpp src/pidgey_feedback.hpp)
                target_link_libraries(isle-pidgey-feedback PUBLIC
                    isle-pidgey-producer isle-canonical-executor isle-photo-feedback)
                isle_target_options(isle-pidgey-feedback)
            endif()
            target_link_libraries(35mmisle-rsp-probe PRIVATE isle-canonical-executor)
            target_compile_definitions(35mmisle-rsp-probe PRIVATE ISLE_HAS_CANONICAL_RDP=1)
            if(ISLE_BUILD_HOST_PROBE AND ISLE_BUILD_GAME_ENGINE)
                target_sources(35mmisle-host-probe PRIVATE src/model_preview.cpp src/model_preview.hpp)
                target_link_libraries(35mmisle-host-probe PRIVATE
                    isle-canonical-executor isle-rsp-fixture isle-required-work isle-pidgey-producer)
                target_compile_definitions(35mmisle-host-probe PRIVATE ISLE_HAS_CANONICAL_RDP=1)
            endif()
        endif()
        if(BUILD_TESTING)
            add_executable(required-work-tests tests/required_work_tests.cpp)
            target_link_libraries(required-work-tests PRIVATE isle-required-work)
            isle_target_options(required-work-tests)
            add_test(NAME required-work-tests COMMAND required-work-tests)
            set_tests_properties(required-work-tests PROPERTIES LABELS "synthetic" TIMEOUT 60)
        endif()
    endif()
    if(BUILD_TESTING)
        add_executable(rsp-tests tests/rsp_tests.cpp)
        target_link_libraries(rsp-tests PRIVATE isle-rsp)
        isle_target_options(rsp-tests)
        add_test(NAME rsp-tests COMMAND rsp-tests)
        set_tests_properties(rsp-tests PROPERTIES LABELS "synthetic;coprocessor" TIMEOUT 60)
    endif()
endif()

if(TARGET isle-rsp AND TARGET isle-game-audio)
    add_executable(game-audio-rsp-smoke tests/game_audio_rsp_smoke.cpp)
    target_link_libraries(game-audio-rsp-smoke PRIVATE isle-game-audio isle-graphics-task isle-rsp)
    isle_target_options(game-audio-rsp-smoke)
endif()
if(TARGET isle-rsp AND TARGET isle-environment-sound)
    add_executable(environment-sound-rsp-smoke tests/environment_sound_rsp_smoke.cpp)
    target_link_libraries(environment-sound-rsp-smoke PRIVATE
        isle-environment-sound isle-graphics-task isle-rsp)
    isle_target_options(environment-sound-rsp-smoke)
endif()
if(TARGET isle-canonical-executor AND TARGET isle-menu-resources)
    add_executable(menu-effects-rsp-smoke tests/menu_effects_rsp_smoke.cpp)
    target_link_libraries(menu-effects-rsp-smoke PRIVATE
        isle-menu-effects isle-canonical-executor isle-rsp-fixture)
    isle_target_options(menu-effects-rsp-smoke)
    add_executable(sprite-producer-rom-smoke tests/sprite_producer_rom_smoke.cpp)
    target_link_libraries(sprite-producer-rom-smoke PRIVATE
        isle-sprite-producer isle-menu-resources isle-graphics-task isle-canonical-executor)
    isle_target_options(sprite-producer-rom-smoke)
endif()
if(TARGET isle-canonical-executor AND TARGET isle-menu-model-producer)
    add_executable(menu-model-rsp-smoke tests/menu_model_rsp_smoke.cpp)
    target_link_libraries(menu-model-rsp-smoke PRIVATE
        isle-menu-model-producer isle-canonical-executor)
    isle_target_options(menu-model-rsp-smoke)
    add_executable(world-producer-rsp-smoke tests/world_producer_rsp_smoke.cpp)
    target_link_libraries(world-producer-rsp-smoke PRIVATE isle-world isle-canonical-executor)
    isle_target_options(world-producer-rsp-smoke)
endif()

if(ISLE_BUILD_AUDIO_PROBE)
    find_package(SDL3 3.4.2 REQUIRED CONFIG COMPONENTS SDL3)
    add_library(isle-audio-output STATIC src/audio_transport.cpp include/35mmisle/audio_transport.hpp)
    target_include_directories(isle-audio-output PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(isle-audio-output PUBLIC SDL3::SDL3)
    isle_target_options(isle-audio-output)

    add_library(isle-audio-probe-support STATIC src/audio_probe_fixture.cpp src/audio_probe_fixture.hpp)
    target_link_libraries(isle-audio-probe-support PUBLIC isle-audio isle-rom-snapshot isle-rsp)
    isle_target_options(isle-audio-probe-support)
    add_executable(35mmisle-audio-probe src/audio_probe.cpp)
    target_link_libraries(35mmisle-audio-probe PRIVATE isle-audio-probe-support isle-audio-output)
    isle_target_options(35mmisle-audio-probe)
    if(MINGW)
        target_link_options(35mmisle-audio-probe PRIVATE -municode)
    endif()
    if(ISLE_BUILD_HOST_PROBE)
        target_sources(35mmisle-host-probe PRIVATE src/audio_host_probe.cpp src/audio_host_probe.hpp)
        target_link_libraries(35mmisle-host-probe PRIVATE isle-audio-probe-support isle-audio-output)
        target_compile_definitions(35mmisle-host-probe PRIVATE ISLE_HAS_AUDIO_PROBE=1)
    endif()
endif()

if(TARGET isle-menu-scene AND TARGET isle-canonical-executor)
    add_library(isle-scene-graphics STATIC src/scene_graphics.cpp src/scene_graphics.hpp src/scene_render.hpp)
    target_link_libraries(isle-scene-graphics PUBLIC
        isle-menu-scene isle-sprite-producer isle-graphics-task isle-canonical-executor)
    isle_target_options(isle-scene-graphics)
endif()
if(TARGET libultraship AND TARGET isle-audio-output AND TARGET isle-rsp AND TARGET isle-game-audio)
    add_library(isle-game-host STATIC src/game_host.cpp src/game_host.hpp)
    target_compile_definitions(isle-game-host PRIVATE F3DEX_GBI_2
        CVAR_ENABLE_MULTI_VIEWPORTS="${CVAR_ENABLE_MULTI_VIEWPORTS}"
        ISLE_HOST_RENDERER_RESOURCES="${ISLE_HOST_RENDERER_RESOURCES}")
    target_link_libraries(isle-game-host PUBLIC isle-game-audio isle-game-input isle-audio-output
        PRIVATE libultraship isle-graphics-task isle-rsp isle-continuation Threads::Threads)
    isle_target_options(isle-game-host)
endif()
if(WIN32 AND TARGET isle-game-host)
    enable_language(RC)
endif()

if(TARGET isle-scene-graphics AND TARGET isle-game-host)
    # Explicit private-ROM scenario, never a ROM-free CTest or installed game.
    add_executable(35mmisle-startup-runtime-rom-smoke tests/startup_runtime_rom_smoke.cpp)
    target_sources(35mmisle-startup-runtime-rom-smoke PRIVATE
        tests/new_game_scene_rom_smoke.cpp tests/oaks_lab_scene_rom_smoke.cpp)
    target_link_libraries(35mmisle-startup-runtime-rom-smoke PRIVATE isle-scene-graphics isle-game-host
        isle-host-clock isle-new-game-scene isle-oaks-lab-scene)
    isle_target_options(35mmisle-startup-runtime-rom-smoke)
    if(WIN32)
        target_sources(35mmisle-startup-runtime-rom-smoke PRIVATE src/game_host.rc)
        target_include_directories(35mmisle-startup-runtime-rom-smoke PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
        set_source_files_properties(src/game_host.rc PROPERTIES
            OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/game_host.manifest")
        if(MSVC)
            target_link_options(35mmisle-startup-runtime-rom-smoke PRIVATE /MANIFEST:NO)
        elseif(MINGW)
            target_link_options(35mmisle-startup-runtime-rom-smoke PRIVATE -municode)
        endif()
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS 35mmisle-rom RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/35mmisle-rom-tool")

set(CPACK_PACKAGE_NAME "35mmisle-rom-tool")
set(CPACK_PACKAGE_VENDOR "35mmIsle contributors")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Read-only ROM identity checker; no game or assets")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-64bit")
set(CPACK_VERBATIM_VARIABLES ON)
# Only install-listed tooling enters a binary archive; never package the source tree.
set(CPACK_SOURCE_GENERATOR "")
foreach(format IN ITEMS 7Z CYGWIN RPM TBZ2 TGZ TXZ TZ ZIP)
    set(CPACK_SOURCE_${format} OFF)
endforeach()
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)
