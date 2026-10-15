cmake_minimum_required(VERSION 3.24)

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
        target_link_libraries(35mmisle-host-probe PRIVATE isle-engine isle-sprite isle-rom isle-vpk0)
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

    # All executables use explicit failure checks, including in Release builds.
    set_tests_properties(native-tests rom-tests vpk0-tests process-tests message-queue-tests continuation-tests sprite-tests
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
