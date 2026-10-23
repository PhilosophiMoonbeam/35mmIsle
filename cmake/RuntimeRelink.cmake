# Private, explicit LGPL static-library relink material. Never installed by CPack.
include_guard(GLOBAL)
option(ISLE_BUILD_RUNTIME_RELINK_KIT "Enable PRIVATE whole-product GNU lightning relink companion" OFF)
set(ISLE_RUNTIME_RELINK_OUTPUT "${CMAKE_BINARY_DIR}/private-runtime-relink-kit" CACHE PATH
    "PRIVATE relink kit output (contains application objects; no redistribution permission implied)")

function(_isle_relink_json_string output value)
    string(REPLACE "\\" "\\\\" value "${value}")
    string(REPLACE "\"" "\\\"" value "${value}")
    string(REPLACE "\n" "\\n" value "${value}")
    string(REPLACE "\r" "\\r" value "${value}")
    string(REPLACE "\t" "\\t" value "${value}")
    set(${output} "\"${value}\"" PARENT_SCOPE)
endfunction()

function(isle_add_runtime_relink_kit)
    if(NOT ISLE_BUILD_RUNTIME_RELINK_KIT)
        return()
    endif()
    if(TARGET runtime-relink-kit)
        message(FATAL_ERROR "isle_add_runtime_relink_kit must be called exactly once")
    endif()
    if(NOT TARGET 35mmisle OR NOT TARGET isle-rsp-lightning OR
       NOT EXISTS "${ISLE_RSP_PROVENANCE}" OR NOT IS_DIRECTORY "${ISLE_RSP_SOURCE_ROOT}")
        message(FATAL_ERROR "Private relink kit requires the actual product and prepared RSP provenance")
    endif()
    if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_GENERATOR STREQUAL "Ninja") AND
       NOT (MSVC AND CMAKE_GENERATOR STREQUAL "Visual Studio 17 2022"))
        message(FATAL_ERROR "Private relink capture requires Linux Ninja or native VS2022/MSBuild")
    endif()
    if(MSVC AND NOT CMAKE_MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        message(FATAL_ERROR "Private relink capture preserves the existing static /MT runtime configuration")
    endif()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(_module "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    get_filename_component(_project "${_module}/.." ABSOLUTE)
    set(_private "${CMAKE_BINARY_DIR}/private-runtime-relink")
    file(MAKE_DIRECTORY "${_private}" "${CMAKE_BINARY_DIR}/.cmake/api/v1/query/client-isle-runtime-relink")
    file(WRITE "${CMAKE_BINARY_DIR}/.cmake/api/v1/query/client-isle-runtime-relink/query.json"
        "{\"requests\":[{\"kind\":\"codemodel\",\"version\":2},{\"kind\":\"toolchains\",\"version\":1}]}\n")
    set(_settings "{}")
    foreach(_pair IN ITEMS
        "build|${CMAKE_BINARY_DIR}" "project|${_project}" "source|${ISLE_RSP_SOURCE_ROOT}"
        "provenance|${ISLE_RSP_PROVENANCE}" "capture|${_private}/capture-$<CONFIG>.json"
        "product|$<TARGET_FILE:35mmisle>" "lightning|$<TARGET_FILE:isle-rsp-lightning>"
        "configuration|$<CONFIG>" "generator|${CMAKE_GENERATOR}" "system|${CMAKE_SYSTEM_NAME}"
        "processor|${CMAKE_SYSTEM_PROCESSOR}" "cmake_version|${CMAKE_VERSION}"
        "cmake|${CMAKE_COMMAND}" "objdump|${CMAKE_OBJDUMP}"
        "c_compiler|${CMAKE_C_COMPILER}" "cxx_compiler|${CMAKE_CXX_COMPILER}"
        "compiler_id|${CMAKE_CXX_COMPILER_ID}" "compiler_version|${CMAKE_CXX_COMPILER_VERSION}"
        "linker|${CMAKE_LINKER}" "msvc_toolset|${CMAKE_VS_PLATFORM_TOOLSET_VERSION}"
        "windows_sdk|${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}" "crt|${CMAKE_MSVC_RUNTIME_LIBRARY}"
        "install_rpath|$<TARGET_PROPERTY:35mmisle,INSTALL_RPATH>")
        string(FIND "${_pair}" "|" _separator)
        string(SUBSTRING "${_pair}" 0 ${_separator} _key)
        math(EXPR _start "${_separator} + 1")
        string(SUBSTRING "${_pair}" ${_start} -1 _value)
        _isle_relink_json_string(_json "${_value}")
        string(JSON _settings SET "${_settings}" "${_key}" "${_json}")
    endforeach()
    file(GENERATE OUTPUT "${_private}/settings-$<CONFIG>.json" CONTENT "${_settings}\n")
    if(MSVC)
        # VS_GLOBAL_* is CMake's documented project-global-property interface.
        # Microsoft.CppCommon.targets passes LinkToolExe/LinkToolPath to Link's
        # ToolExe/ToolPath. This wrapper forwards the ORIGINAL command tail;
        # Python expands response files while MSBuild's temporary files exist.
        foreach(_key IN ITEMS PYTHON SCRIPT SETTINGS LINKER)
            if(_key STREQUAL "PYTHON")
                set(_value "${Python3_EXECUTABLE}")
            elseif(_key STREQUAL "SCRIPT")
                set(_value "${_project}/tools/runtime_relink.py")
            elseif(_key STREQUAL "SETTINGS")
                set(_value "${_private}/settings-$<CONFIG>.json")
            else()
                set(_value "${CMAKE_LINKER}")
            endif()
            string(REPLACE "\\" "/" _value "${_value}")
            string(REPLACE "\"" "\\\"" _value "${_value}")
            set(ISLE_RELINK_${_key} "${_value}")
        endforeach()
        configure_file("${_module}/RuntimeRelinkLinkWrapper.c.in" "${_private}/link-wrapper.c.in" @ONLY)
        file(GENERATE OUTPUT "${_private}/link-wrapper-$<CONFIG>.c" INPUT "${_private}/link-wrapper.c.in")
        add_executable(isle-runtime-relink-link-wrapper EXCLUDE_FROM_ALL "${_private}/link-wrapper-$<CONFIG>.c")
        set_target_properties(isle-runtime-relink-link-wrapper PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
            RUNTIME_OUTPUT_DIRECTORY "${_private}/wrapper/$<CONFIG>"
            OUTPUT_NAME "isle-runtime-relink-link")
        target_compile_options(isle-runtime-relink-link-wrapper PRIVATE /utf-8)
        add_dependencies(35mmisle isle-runtime-relink-link-wrapper)
        set_property(TARGET 35mmisle PROPERTY VS_GLOBAL_LinkToolExe "isle-runtime-relink-link.exe")
        set_property(TARGET 35mmisle PROPERTY VS_GLOBAL_LinkToolPath "${_private}/wrapper/$(Configuration)/")
    else()
        get_property(_launcher TARGET 35mmisle PROPERTY CXX_LINKER_LAUNCHER)
        if(_launcher)
            message(FATAL_ERROR "Private relink capture will not silently replace an existing linker launcher")
        endif()
        # Launcher generator expressions require CMake >=3.27. This target is
        # single-config Ninja: use the concrete configuration with older CMake.
        set_property(TARGET 35mmisle PROPERTY CXX_LINKER_LAUNCHER
            "${Python3_EXECUTABLE};-B;${_project}/tools/runtime_relink.py;capture;--settings;${_private}/settings-${CMAKE_BUILD_TYPE}.json;--")
    endif()
    add_custom_target(runtime-relink-kit
        COMMAND "${Python3_EXECUTABLE}" -B "${_project}/tools/runtime_relink.py" assemble
            --settings "${_private}/settings-$<CONFIG>.json"
            --output "${ISLE_RUNTIME_RELINK_OUTPUT}/$<CONFIG>"
        DEPENDS 35mmisle
        COMMENT "Assembling PRIVATE whole-product relink material; distribution remains R1 HOLD"
        VERBATIM)
endfunction()
