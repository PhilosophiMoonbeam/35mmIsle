# Host-only dependency composition. Standalone tools never include this file.
include_guard(GLOBAL)
include(FetchContent)
find_package(Git REQUIRED)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/dependencies.json"
    "${CMAKE_CURRENT_LIST_DIR}/patches/lus-host-lifetime.patch")

if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Windows)$" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The procedural LUS host probe supports Linux x64 and Windows x64 only")
endif()
if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR "The Windows host probe requires the MSVC x64 toolchain and static-runtime dependencies")
endif()
if(WIN32)
    # Compiler IDs reflect the selected VS platform or Ninja compiler environment,
    # unlike CMAKE_SYSTEM_PROCESSOR, which can still describe the Windows host.
    string(TOLOWER "${CMAKE_C_COMPILER_ARCHITECTURE_ID}" _isle_c_target_arch)
    string(TOLOWER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" _isle_cxx_target_arch)
    if(NOT _isle_c_target_arch STREQUAL "x64" OR NOT _isle_cxx_target_arch STREQUAL "x64")
        message(FATAL_ERROR "The Windows host probe requires x64 C and C++ compiler targets")
    endif()
    if(DEFINED CMAKE_VS_PLATFORM_NAME)
        string(TOLOWER "${CMAKE_VS_PLATFORM_NAME}" _isle_vs_target_arch)
        if(NOT _isle_vs_target_arch STREQUAL "x64")
            message(FATAL_ERROR "The Windows host probe requires the x64 Visual Studio target platform")
        endif()
    endif()
else()
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _isle_target_processor)
    if(NOT _isle_target_processor MATCHES "^(x86_64|amd64|x64)$")
        message(FATAL_ERROR "The Linux host probe requires an x64 target processor")
    endif()
endif()

set(_isle_lus_pin 759b13401cd6903daa67a00a753066f791622e78)
set(ISLE_LUS_SOURCE_DIR "" CACHE PATH "Read-only clean git checkout at the exact LUS pin; empty fetches that pin")
file(READ "${CMAKE_CURRENT_LIST_DIR}/dependencies.json" _isle_dependency_manifest)
string(JSON _isle_manifest_pin GET "${_isle_dependency_manifest}" lus commit)
if(NOT _isle_manifest_pin STREQUAL _isle_lus_pin)
    message(FATAL_ERROR "Libultraship.cmake and dependencies.json disagree on the audited LUS pin")
endif()

# Reject contradictory cache settings rather than quietly overriding the caller.
foreach(_option IN ITEMS NON_PORTABLE INCLUDE_PROFILING COMPONENT_THREAD_SAFE ENABLE_SCRIPTING
        ENABLE_TCC_COMPILER INCLUDE_MPQ_SUPPORT USE_OPENGLES GFX_DEBUG_DISASSEMBLER
        LUS_BUILD_TESTS USE_AUTO_VCPKG PRISM_STANDALONE)
    if(DEFINED ${_option} AND ${_option})
        message(FATAL_ERROR "The host probe requires ${_option}=OFF")
    endif()
    set(${_option} OFF CACHE BOOL "Pinned 35mmIsle host-probe dependency scope" FORCE)
endforeach()
if(DEFINED DISABLE_DLL_LOADER AND NOT DISABLE_DLL_LOADER)
    message(FATAL_ERROR "The host probe requires DISABLE_DLL_LOADER=ON")
endif()
set(DISABLE_DLL_LOADER ON CACHE BOOL "No host-probe DLL loading" FORCE)
if(DEFINED GBI_UCODE AND NOT GBI_UCODE STREQUAL "F3DEX_GBI_2")
    message(FATAL_ERROR "The procedural probe requires GBI_UCODE=F3DEX_GBI_2 (not an ON/OFF value)")
endif()
set(GBI_UCODE F3DEX_GBI_2 CACHE STRING "Procedural probe GBI, not a game compatibility claim" FORCE)
foreach(_part IN ITEMS EPOCH MAJOR MINOR PATCH)
    if(_part STREQUAL "EPOCH")
        set(_value 2)
    else()
        set(_value 0)
    endif()
    if(DEFINED LUS_VERSION_${_part} AND NOT "${LUS_VERSION_${_part}}" STREQUAL "${_value}")
        message(FATAL_ERROR "The host probe records LUS_VERSION_${_part}=${_value}")
    endif()
    set(LUS_VERSION_${_part} ${_value} CACHE STRING "Pinned host LUS API version" FORCE)
endforeach()

# Never allow upstream's patch command to reset a user-supplied dependency checkout.
foreach(_dependency IN ITEMS IMGUI THREADPOOL PRISM MONOCYPHER LIBULTRASHIP)
    if(FETCHCONTENT_SOURCE_DIR_${_dependency})
        message(FATAL_ERROR "Use ISLE_LUS_SOURCE_DIR, not FETCHCONTENT_SOURCE_DIR_${_dependency}; dependency worktrees must be build-owned")
    endif()
endforeach()
if(ISLE_LUS_SOURCE_DIR)
    file(REAL_PATH "${ISLE_LUS_SOURCE_DIR}" _isle_lus_input)
else()
    FetchContent_Declare(isle_lus_source
        GIT_REPOSITORY https://github.com/Kenix3/libultraship.git
        GIT_TAG ${_isle_lus_pin}
        GIT_SUBMODULES "")
    FetchContent_GetProperties(isle_lus_source)
    if(NOT isle_lus_source_POPULATED)
        # Deliberate population without running upstream CMake until the snapshot is isolated.
        FetchContent_Populate(isle_lus_source)
    endif()
    set(_isle_lus_input "${isle_lus_source_SOURCE_DIR}")
endif()
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${_isle_lus_input}" rev-parse HEAD
    OUTPUT_VARIABLE _isle_actual_pin OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _isle_git_result)
if(NOT _isle_git_result EQUAL 0 OR NOT _isle_actual_pin STREQUAL _isle_lus_pin)
    message(FATAL_ERROR "LUS must be an immutable git checkout at ${_isle_lus_pin}")
endif()
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${_isle_lus_input}" status --porcelain --untracked-files=all
    OUTPUT_VARIABLE _isle_dirty RESULT_VARIABLE _isle_git_result)
if(NOT _isle_git_result EQUAL 0 OR NOT _isle_dirty STREQUAL "")
    message(FATAL_ERROR "The selected LUS checkout must be clean; it is never modified by this project")
endif()

# Export committed bytes only. All upstream generated files/patches stay in this build.
set(_isle_lus_private "${CMAKE_BINARY_DIR}/_deps/isle-lus-private")
file(MAKE_DIRECTORY "${_isle_lus_private}")
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${_isle_lus_input}" archive --format=tar
    "--output=${CMAKE_BINARY_DIR}/_deps/isle-lus-pinned.tar" ${_isle_lus_pin}
    RESULT_VARIABLE _isle_archive_result)
if(NOT _isle_archive_result EQUAL 0)
    message(FATAL_ERROR "Could not export the immutable LUS source snapshot")
endif()
file(ARCHIVE_EXTRACT INPUT "${CMAKE_BINARY_DIR}/_deps/isle-lus-pinned.tar" DESTINATION "${_isle_lus_private}")
set(_isle_lifetime_patch "${CMAKE_CURRENT_LIST_DIR}/patches/lus-host-lifetime.patch")
# Exact-pin, three-context-line hunks are applied after each fresh committed-byte
# export. Ship::ControlDeck owns the caches used by its LUS::ControlDeck subclass.
# Run outside any containing worktree: patch paths are rooted at this snapshot,
# never interpreted relative to the parent project or read-only reference checkout.
execute_process(COMMAND "${CMAKE_COMMAND}" -E env --unset=GIT_DIR --unset=GIT_WORK_TREE
    --unset=GIT_COMMON_DIR --unset=GIT_INDEX_FILE
    "GIT_CEILING_DIRECTORIES=${CMAKE_BINARY_DIR}" "${GIT_EXECUTABLE}" apply "${_isle_lifetime_patch}"
    WORKING_DIRECTORY "${_isle_lus_private}" RESULT_VARIABLE _isle_patch_result)
if(NOT _isle_patch_result EQUAL 0)
    message(FATAL_ERROR "The exact-base host lifetime adaptation could not be applied")
endif()
file(SHA256 "${_isle_lifetime_patch}" _isle_lifetime_digest)
file(SHA256 "${CMAKE_BINARY_DIR}/_deps/isle-lus-pinned.tar" _isle_lus_archive_digest)

# These are pinned MIT renderer support shaders, not game assets. Mount only the
# exercised backend's tree, rooted exactly as the renderer's resource identifiers.
set(ISLE_HOST_RENDERER_RESOURCES "${CMAKE_BINARY_DIR}/host-renderer-resources")
file(MAKE_DIRECTORY "${ISLE_HOST_RENDERER_RESOURCES}/shaders")
file(COPY "${_isle_lus_private}/src/fast/shaders/opengl"
    DESTINATION "${ISLE_HOST_RENDERER_RESOURCES}/shaders")
file(COPY "${_isle_lus_private}/LICENSE" DESTINATION "${ISLE_HOST_RENDERER_RESOURCES}")
# Preserve the renderer authors' separate MIT notice alongside the project license.
configure_file("${_isle_lus_private}/src/fast/LICENSE.txt"
    "${ISLE_HOST_RENDERER_RESOURCES}/fast-renderer-LICENSE.txt" COPYONLY)

# Upstream downloads stb without a digest. Replace precisely that acquisition statement
# in the private snapshot; this is an acquisition check, not a source behavior change.
set(_isle_common "${_isle_lus_private}/cmake/dependencies/common.cmake")
file(READ "${_isle_common}" _isle_common_text)
set(_isle_stb_old [=[file(DOWNLOAD "https://github.com/nothings/stb/raw/0bc88af4de5fb022db643c2d8e549a0927749354/stb_image.h" "${STB_DIR}/stb_image.h")]=])
set(_isle_stb_new [=[file(DOWNLOAD "https://raw.githubusercontent.com/nothings/stb/0bc88af4de5fb022db643c2d8e549a0927749354/stb_image.h" "${STB_DIR}/stb_image.h" EXPECTED_HASH SHA256=c54b15a689e6a1f32c75e2ec23afa442e3e0e37e894b73c1974d08679b20dd5c TLS_VERIFY ON)]=])
string(FIND "${_isle_common_text}" "${_isle_stb_old}" _isle_stb_location)
if(_isle_stb_location LESS 0)
    message(FATAL_ERROR "The pinned stb acquisition statement changed unexpectedly")
endif()
string(REPLACE "${_isle_stb_old}" "${_isle_stb_new}" _isle_common_text "${_isle_common_text}")
file(WRITE "${_isle_common}" "${_isle_common_text}")

# First FetchContent declaration wins. Resolve upstream tag names to recorded commits.
FetchContent_Declare(ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG 4806a1924ff6181180bf5e4b8b79ab4394118875
    PATCH_COMMAND "${CMAKE_COMMAND}"
        "-Dpatch_file=${_isle_lus_private}/cmake/dependencies/patches/imgui-fixes-and-config.patch"
        -Dwith_reset=FALSE -P "${_isle_lus_private}/cmake/dependencies/git-patch.cmake")
FetchContent_Declare(ThreadPool GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
    GIT_TAG 097aa718f25d44315cadb80b407144ad455ee4f9)
FetchContent_Declare(prism GIT_REPOSITORY https://github.com/KiritoDv/prism-processor.git
    GIT_TAG 1de054450e7b3c5f777d2e3dfcb228ad120c329d)
FetchContent_Declare(monocypher GIT_REPOSITORY https://github.com/LoupVaillant/Monocypher.git
    GIT_TAG 0d85f98c9d9b0227e42cf795cb527dff372b40a4)

# Fail at discovery, not as an apparently ready Null backend or an opaque link error.
find_package(OpenGL REQUIRED)
find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3)
find_package(libzip REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(spdlog REQUIRED)
if(WIN32)
    find_package(GLEW REQUIRED)
endif()
add_subdirectory("${_isle_lus_private}" "${CMAKE_BINARY_DIR}/_deps/isle-lus-build" EXCLUDE_FROM_ALL)
# Upstream applies this privately. Export the same producer definition deliberately.
target_compile_definitions(libultraship PUBLIC F3DEX_GBI_2)
if(NOT WIN32)
    target_link_libraries(libultraship PUBLIC OpenGL::GL Threads::Threads ${CMAKE_DL_LIBS})
endif()

# Build-local evidence, not a claim that the system package composition is fully locked.
file(WRITE "${CMAKE_BINARY_DIR}/isle-host-dependencies.txt"
    "LUS=${_isle_lus_pin}\nLUS_API_VERSION=2.0.0.0\n"
    "LUS_source=immutable git archive; host_patch_scope=build-private snapshot only\n"
    "LUS_archive_sha256=${_isle_lus_archive_digest}\n"
    "host_lifetime_patch_sha256=${_isle_lifetime_digest}\n"
    "ImGui=4806a1924ff6181180bf5e4b8b79ab4394118875 + LUS imgui-fixes-and-config.patch\n"
    "ThreadPool=097aa718f25d44315cadb80b407144ad455ee4f9\n"
    "prism=1de054450e7b3c5f777d2e3dfcb228ad120c329d\n"
    "monocypher=0d85f98c9d9b0227e42cf795cb527dff372b40a4\n"
    "stb=0bc88af4de5fb022db643c2d8e549a0927749354 sha256=c54b15a689e6a1f32c75e2ec23afa442e3e0e37e894b73c1974d08679b20dd5c\n"
    "GBI_UCODE=F3DEX_GBI_2; producer=F3DEX_GBI_2; decoder=ucode_f3dex2; GBI_FLOATS=undefined\n"
    "NON_PORTABLE=OFF; INCLUDE_PROFILING=OFF; COMPONENT_THREAD_SAFE=OFF\n"
    "ENABLE_SCRIPTING=OFF; ENABLE_TCC_COMPILER=OFF; INCLUDE_MPQ_SUPPORT=OFF; DISABLE_DLL_LOADER=ON\n"
    "USE_OPENGLES=OFF; GFX_DEBUG_DISASSEMBLER=OFF; LUS_BUILD_TESTS=OFF; USE_AUTO_VCPKG=OFF\n"
    "SDL3=${SDL3_VERSION}; libzip=${libzip_VERSION}; nlohmann_json=${nlohmann_json_VERSION}\n"
    "tinyxml2=${tinyxml2_VERSION}; spdlog=${spdlog_VERSION}; GLEW=${GLEW_VERSION}\n"
    "C=${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}; CXX=${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\n"
    "System packages, platform SDK and transitive binary content still require separate inventory.\n")
message(STATUS "Nonplayable LUS renderer: ${_isle_lus_pin}, explicit F3DEX_GBI_2, SDL3/OpenGL; private intro mode requires the separate game-engine option; no audio")
