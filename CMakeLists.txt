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
        target_compile_options(${target} PRIVATE /W4 /utf-8 /fp:strict
            "$<$<COMPILE_LANGUAGE:CXX>:/permissive->"
            "$<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>")
    else()
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow;-fno-fast-math;-ffp-contract=off>"
            "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow;-fno-fast-math;-ffp-contract=off>")
    endif()
    if(ISLE_ENABLE_SANITIZERS)
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=all)
    endif()
endfunction()

add_library(isle-native STATIC src/native.c include/35mmisle/native.h)
target_include_directories(isle-native PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
isle_target_options(isle-native)

add_library(isle-rom STATIC src/rom.cpp include/35mmisle/rom.hpp)
target_include_directories(isle-rom PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
isle_target_options(isle-rom)

if(ISLE_BUILD_RSP_PROBE OR ISLE_BUILD_AUDIO_PROBE OR (ISLE_BUILD_HOST_PROBE AND ISLE_BUILD_GAME_ENGINE))
    add_library(isle-rom-snapshot STATIC src/rom_snapshot.cpp src/rom_snapshot.hpp)
    target_link_libraries(isle-rom-snapshot PUBLIC isle-rom)
    isle_target_options(isle-rom-snapshot)
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

add_library(isle-audio-bank STATIC src/audio_bank.cpp include/35mmisle/audio_bank.hpp)
target_include_directories(isle-audio-bank PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
isle_target_options(isle-audio-bank)

add_library(isle-audio-sfx STATIC src/audio_sfx.cpp include/35mmisle/audio_sfx.hpp)
target_link_libraries(isle-audio-sfx PUBLIC isle-audio-bank)
isle_target_options(isle-audio-sfx)

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
    target_link_libraries(35mmisle-host-probe PRIVATE libultraship Threads::Threads)
    isle_target_options(35mmisle-host-probe)
endif()

include(CTest)
if(ISLE_BUILD_GAME_ENGINE)
    include(cmake/SnapEngine.cmake)
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

    add_executable(audio-sfx-tests tests/audio_sfx_tests.cpp)
    target_link_libraries(audio-sfx-tests PRIVATE isle-audio-sfx)
    isle_target_options(audio-sfx-tests)
    add_test(NAME audio-sfx-tests COMMAND audio-sfx-tests)

    # All executables use explicit failure checks, including in Release builds.
    set_tests_properties(native-tests rom-tests vpk0-tests process-tests message-queue-tests continuation-tests sprite-tests audio-bank-tests audio-sfx-tests
        PROPERTIES LABELS "synthetic" TIMEOUT 60)
endif()

if(ISLE_BUILD_RDP_PROBE)
    include(cmake/ParallelRdp.cmake)
    add_executable(35mmisle-rdp-probe src/rdp_probe.cpp src/rdp_fixture.cpp src/rdp_fixture.hpp)
    target_link_libraries(35mmisle-rdp-probe PRIVATE isle_parallel_rdp)
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
        add_executable(35mmisle-rsp-probe src/rsp_probe.cpp src/rsp_fixture.cpp src/rsp_fixture.hpp)
        target_link_libraries(35mmisle-rsp-probe PRIVATE isle-rsp isle-rom-snapshot)
        isle_target_options(35mmisle-rsp-probe)
        if(ISLE_BUILD_RDP_PROBE)
            target_sources(35mmisle-rsp-probe PRIVATE src/rsp_render.cpp src/rsp_render.hpp)
            target_link_libraries(35mmisle-rsp-probe PRIVATE isle_parallel_rdp)
            target_compile_definitions(35mmisle-rsp-probe PRIVATE ISLE_HAS_CANONICAL_RDP=1)
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

if(ISLE_BUILD_AUDIO_PROBE)
    find_package(SDL3 3.4.2 REQUIRED CONFIG COMPONENTS SDL3)
    add_library(isle-audio-output STATIC src/audio_transport.cpp include/35mmisle/audio_transport.hpp)
    target_include_directories(isle-audio-output PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(isle-audio-output PUBLIC SDL3::SDL3)
    isle_target_options(isle-audio-output)

    add_library(isle-audio-probe-support STATIC src/audio_probe_fixture.cpp src/audio_probe_fixture.hpp)
    target_link_libraries(isle-audio-probe-support PUBLIC isle-audio-sfx isle-rom-snapshot isle-rsp)
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
