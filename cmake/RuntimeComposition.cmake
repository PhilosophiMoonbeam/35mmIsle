# Call once, AFTER every Runtime payload/notice/dependency install rule.
# Inputs stay build-private; only the deliberately bounded install-time record ships.
include_guard(GLOBAL)
set(_isle_composition_module_directory "${CMAKE_CURRENT_LIST_DIR}")

function(_isle_composition_string object key value)
    string(REPLACE "\\" "\\\\" escaped "${value}")
    string(REPLACE "\"" "\\\"" escaped "${escaped}")
    string(REPLACE "\n" "\\n" escaped "${escaped}")
    string(REPLACE "\r" "\\r" escaped "${escaped}")
    string(REPLACE "\t" "\\t" escaped "${escaped}")
    string(JSON result SET "${${object}}" "${key}" "\"${escaped}\"")
    set("${object}" "${result}" PARENT_SCOPE)
endfunction()

function(isle_install_runtime_composition)
    if(NOT TARGET 35mmisle)
        message(FATAL_ERROR "Runtime composition requires the actual native product")
    endif()
    find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)
    find_package(Git REQUIRED)
    set(ISLE_RUNTIME_SDK_IDENTITIES "" CACHE FILEPATH
        "Schema-2 source-build or exact Debian-binary identities for reached non-dpkg Linux DSOs")
    get_property(_multi_configuration GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(_multi_configuration)
        set(_multi_configuration true)
    else()
        set(_multi_configuration false)
    endif()
    set(inputs "{}")
    foreach(pair IN ITEMS
            "source_root|${PROJECT_SOURCE_DIR}" "build_root|${CMAKE_BINARY_DIR}"
            "project_name|${PROJECT_NAME}" "project_version|${PROJECT_VERSION}"
            "system|${CMAKE_SYSTEM_NAME}" "processor|${CMAKE_SYSTEM_PROCESSOR}"
            "configuration|$<CONFIG>" "pointer_bytes|${CMAKE_SIZEOF_VOID_P}"
            "generator|${CMAKE_GENERATOR}" "cmake_version|${CMAKE_VERSION}"
            "multi_configuration|${_multi_configuration}"
            "cmake|${CMAKE_COMMAND}" "git|${GIT_EXECUTABLE}"
            "c_compiler|${CMAKE_C_COMPILER}" "c_id|${CMAKE_C_COMPILER_ID}"
            "c_version|${CMAKE_C_COMPILER_VERSION}"
            "c_architecture|${CMAKE_C_COMPILER_ARCHITECTURE_ID}"
            "c_target|${CMAKE_C_COMPILER_TARGET}"
            "cxx_compiler|${CMAKE_CXX_COMPILER}" "cxx_id|${CMAKE_CXX_COMPILER_ID}"
            "cxx_version|${CMAKE_CXX_COMPILER_VERSION}"
            "cxx_architecture|${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}"
            "cxx_target|${CMAKE_CXX_COMPILER_TARGET}"
            "linker|${CMAKE_LINKER}" "archiver|${CMAKE_AR}"
            "toolchain|${CMAKE_TOOLCHAIN_FILE}"
            "msvc_toolset|${MSVC_TOOLSET_VERSION}"
            "msvc_runtime|$<GENEX_EVAL:$<TARGET_PROPERTY:35mmisle,MSVC_RUNTIME_LIBRARY>>"
            "windows_sdk_version|${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}"
            "windows_sdk_environment_version|$ENV{WindowsSDKVersion}"
            "windows_sdk_root|$ENV{WindowsSdkDir}"
            "vcpkg_installed|${VCPKG_INSTALLED_DIR}"
            "vcpkg_triplet|${VCPKG_TARGET_TRIPLET}"
            "vcpkg_root|$ENV{ISLE_WINDOWS_VCPKG_ROOT}"
            "sdk_identities|${ISLE_RUNTIME_SDK_IDENTITIES}"
            "lus_input|${_isle_lus_input}" "lus_source|${_isle_lus_private}"
            "lus_archive|${_isle_lus_archive}"
            "lus_inventory|${_isle_lus_inventory}"
            "rsp_root|${_isle_rsp_private}" "rdp_root|${_isle_rdp_private}"
            "product_relative|${CMAKE_INSTALL_BINDIR}/$<TARGET_FILE_NAME:35mmisle>"
            "checker_relative|${CMAKE_INSTALL_BINDIR}/$<TARGET_FILE_NAME:35mmisle-rom>"
            "notice_relative|${CMAKE_INSTALL_DATADIR}/licenses/35mmisle"
            "manifest_relative|${CMAKE_INSTALL_DATADIR}/35mmisle/runtime-composition.json")
        string(FIND "${pair}" "|" separator)
        string(SUBSTRING "${pair}" 0 ${separator} key)
        math(EXPR start "${separator} + 1")
        string(SUBSTRING "${pair}" ${start} -1 value)
        _isle_composition_string(inputs "${key}" "${value}")
    endforeach()
    # Flags are fingerprinted privately, never exported with host include paths.
    set(configurations ${CMAKE_CONFIGURATION_TYPES})
    if(NOT configurations AND CMAKE_BUILD_TYPE)
        list(APPEND configurations "${CMAKE_BUILD_TYPE}")
    endif()
    foreach(language IN ITEMS C CXX)
        set(flags "${CMAKE_${language}_FLAGS}")
        foreach(configuration IN LISTS configurations)
            string(TOUPPER "${configuration}" configuration_upper)
            string(APPEND flags "$<$<CONFIG:${configuration}>: ${CMAKE_${language}_FLAGS_${configuration_upper}}>")
        endforeach()
        _isle_composition_string(inputs "${language}_flags" "${flags}")
    endforeach()
    set(dependencies "[]")
    set(index 0)
    foreach(dependency IN ITEMS ImGui ThreadPool prism monocypher)
        FetchContent_GetProperties(${dependency} SOURCE_DIR selected_source)
        set(entry "{}")
        _isle_composition_string(entry name "${dependency}")
        _isle_composition_string(entry source "${selected_source}")
        string(JSON dependencies SET "${dependencies}" ${index} "${entry}")
        math(EXPR index "${index} + 1")
    endforeach()
    string(JSON inputs SET "${inputs}" fetched "${dependencies}")
    set(packages "[]")
    set(index 0)
    foreach(package IN ITEMS SDL3 libzip nlohmann_json tinyxml2 spdlog GLEW)
        if(package STREQUAL "GLEW" AND NOT WIN32)
            continue()
        endif()
        set(entry "{}")
        _isle_composition_string(entry name "${package}")
        _isle_composition_string(entry version "${${package}_VERSION}")
        _isle_composition_string(entry config_directory "${${package}_DIR}")
        string(JSON packages SET "${packages}" ${index} "${entry}")
        math(EXPR index "${index} + 1")
    endforeach()
    string(JSON inputs SET "${inputs}" packages "${packages}")
    set(_isle_composition_inputs "${CMAKE_CURRENT_BINARY_DIR}/runtime-composition-inputs-$<CONFIG>.json")
    file(GENERATE OUTPUT "${_isle_composition_inputs}" CONTENT "${inputs}\n")
    set(_isle_composition_python "${Python3_EXECUTABLE}")
    set(_isle_composition_generator "${PROJECT_SOURCE_DIR}/tools/runtime_composition.py")
    configure_file("${_isle_composition_module_directory}/RuntimeComposition.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/RuntimeComposition.cmake.in" @ONLY)
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/RuntimeComposition-$<CONFIG>.cmake"
        INPUT "${CMAKE_CURRENT_BINARY_DIR}/RuntimeComposition.cmake.in")
    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/RuntimeComposition-$<CONFIG>.cmake"
        COMPONENT Runtime)
endfunction()
