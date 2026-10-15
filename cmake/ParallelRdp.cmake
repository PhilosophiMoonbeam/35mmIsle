# Renderer-only synthetic headless probe. No reference implementation is linked.
include_guard(GLOBAL)
include(ExternalProject)
find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)
if(CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "The RDP probe builds its pinned host shader compiler; cross-compilation is not admitted")
endif()
if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Windows)$" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The bounded RDP probe currently admits Linux/Windows 64-bit native builds")
endif()

set(_isle_rdp_manifest "${CMAKE_CURRENT_LIST_DIR}/rdp-dependencies.json")
set(_isle_rdp_prepare "${CMAKE_CURRENT_LIST_DIR}/../tools/prepare_rdp.py")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_isle_rdp_manifest}" "${_isle_rdp_prepare}")
file(SHA256 "${_isle_rdp_manifest}" _isle_rdp_manifest_digest)
if(NOT _isle_rdp_manifest_digest STREQUAL "1fea5cb6873bbfa97e2924f400bd016c0f01a499526957f3e20efc6df99d0bbc")
    message(FATAL_ERROR "The audited RDP/tool dependency manifest changed; floating/substituted inputs are not admitted")
endif()
# There is deliberately no alternate checkout or precompiled slangmosh interface.
foreach(_override ISLE_RDP_SOURCE_DIR ISLE_GRANITE_SOURCE_DIR ISLE_SLANGMOSH_EXECUTABLE
        FETCHCONTENT_SOURCE_DIR_PARALLEL_RDP FETCHCONTENT_SOURCE_DIR_GRANITE)
    if(DEFINED ${_override} AND NOT "${${_override}}" STREQUAL "")
        message(FATAL_ERROR "${_override} is not admitted: RDP inputs must come from the exact digest-checked manifest")
    endif()
endforeach()
set(_isle_rdp_private "${CMAKE_BINARY_DIR}/_deps/isle-rdp-private")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${_isle_rdp_prepare}"
    --manifest "${_isle_rdp_manifest}" --output "${_isle_rdp_private}"
    RESULT_VARIABLE _isle_rdp_prepare_result)
if(NOT _isle_rdp_prepare_result EQUAL 0)
    message(FATAL_ERROR "Could not materialize exact renderer/compiler inputs; see pinned RDP preparation error above")
endif()

set(_isle_rdp_tool_build "${CMAKE_BINARY_DIR}/_deps/isle-rdp-shader-tool-build")
set(_isle_slangmosh "${_isle_rdp_tool_build}/bin/slangmosh${CMAKE_EXECUTABLE_SUFFIX}")
ExternalProject_Add(isle_rdp_shader_compiler
    SOURCE_DIR "${_isle_rdp_private}/compiler"
    BINARY_DIR "${_isle_rdp_tool_build}"
    DOWNLOAD_COMMAND "" UPDATE_COMMAND "" PATCH_COMMAND ""
    CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Release"
        "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}" "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DPython3_EXECUTABLE=${Python3_EXECUTABLE}"
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
    BUILD_COMMAND "${CMAKE_COMMAND}" -E env "GIT_CEILING_DIRECTORIES=${_isle_rdp_private}"
        "${CMAKE_COMMAND}" --build <BINARY_DIR> --config Release --target slangmosh
    BUILD_ALWAYS TRUE
    BUILD_BYPRODUCTS "${_isle_slangmosh}"
    INSTALL_COMMAND "" TEST_COMMAND "")

set(_isle_rdp_runtime "${_isle_rdp_private}/renderer")
set(_isle_rdp_shader_header "${_isle_rdp_runtime}/parallel-rdp/shaders/slangmosh.hpp")
file(GLOB _isle_rdp_shader_inputs "${_isle_rdp_runtime}/parallel-rdp/shaders/*")
list(APPEND _isle_rdp_shader_inputs
    "${_isle_rdp_runtime}/Granite/assets/shaders/inc/debug_channel.h")
list(REMOVE_ITEM _isle_rdp_shader_inputs "${_isle_rdp_shader_header}")
add_custom_command(OUTPUT "${_isle_rdp_shader_header}"
    COMMAND "${_isle_slangmosh}" --strip -O --namespace RDP
        parallel-rdp/shaders/slangmosh.json --output "${_isle_rdp_shader_header}"
    WORKING_DIRECTORY "${_isle_rdp_runtime}"
    DEPENDS isle_rdp_shader_compiler ${_isle_rdp_shader_inputs}
    COMMENT "Compiling exact pinned RDP GLSL to embedded SPIR-V with the pinned compiler closure"
    VERBATIM)
add_custom_target(isle_rdp_embedded_shaders DEPENDS "${_isle_rdp_shader_header}")

# This is precisely the upstream standalone config.mk source composition, not
# Granite's application/WSI stack and never rdp-utils/alp-core/Angrylion.
file(GLOB _isle_rdp_sources "${_isle_rdp_runtime}/parallel-rdp/*.cpp")
foreach(_unit buffer buffer_pool command_buffer command_pool context cookie descriptor_set
        device event_manager fence fence_manager image indirect_layout memory_allocator
        pipeline_event query_pool render_pass sampler semaphore semaphore_manager shader)
    list(APPEND _isle_rdp_sources "${_isle_rdp_runtime}/vulkan/${_unit}.cpp")
endforeach()
foreach(_unit arena_allocator logging thread_id aligned_alloc timer timeline_trace_file environment thread_name)
    list(APPEND _isle_rdp_sources "${_isle_rdp_runtime}/util/${_unit}.cpp")
endforeach()
list(APPEND _isle_rdp_sources "${_isle_rdp_runtime}/vulkan/texture/texture_format.cpp"
    "${_isle_rdp_runtime}/volk/volk.c" "${_isle_rdp_shader_header}")
add_library(isle_parallel_rdp STATIC ${_isle_rdp_sources})
add_dependencies(isle_parallel_rdp isle_rdp_embedded_shaders)
target_compile_features(isle_parallel_rdp PUBLIC cxx_std_17)
target_include_directories(isle_parallel_rdp SYSTEM PUBLIC
    "${_isle_rdp_runtime}/parallel-rdp" "${_isle_rdp_runtime}/volk"
    "${_isle_rdp_runtime}/vulkan" "${_isle_rdp_runtime}/vulkan-headers/include"
    "${_isle_rdp_runtime}/util")
target_link_libraries(isle_parallel_rdp PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
if(WIN32)
    target_compile_definitions(isle_parallel_rdp PUBLIC VK_USE_PLATFORM_WIN32_KHR)
    target_link_libraries(isle_parallel_rdp PUBLIC winmm)
endif()
# This bounded development probe is not installed or packaged. Preserve notices
# and provenance in its build-private tree; any future binary redistribution
# must separately include and review that actual composition.
message(STATUS "RDP probe recipe: pinned renderer + build-local pinned slangmosh/compiler; runtime Vulkan support is checked by the probe, not assumed")
