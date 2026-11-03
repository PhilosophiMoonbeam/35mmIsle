# Source-backed native LLVM ORC dependency. Acquisition/configure/build is an
# explicit provisioning step, never hidden in a project configure operation.
include_guard(GLOBAL)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(ISLE_LLVM_ROOT "${CMAKE_BINARY_DIR}/private-llvm" CACHE PATH
    "Owned LLVM 21.1.8 dependency root prepared with tools/prepare_llvm.py")
foreach(_override IN ITEMS LLVM_ROOT LLVM_CONFIG LLVM_CONFIG_EXECUTABLE
        ISLE_LLVM_SOURCE_DIR FETCHCONTENT_SOURCE_DIR_LLVM)
    if(DEFINED ${_override} AND NOT "${${_override}}" STREQUAL "")
        message(FATAL_ERROR "Unverified LLVM override ${_override} is forbidden; use ISLE_LLVM_ROOT with tools/prepare_llvm.py")
    endif()
endforeach()
if(CMAKE_CROSSCOMPILING OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR
        NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Windows)$")
    message(FATAL_ERROR "LLVM RSP requires a native Linux or Windows x64 compiler")
endif()
file(REAL_PATH "${ISLE_LLVM_ROOT}" ISLE_LLVM_ROOT)
set(ISLE_LLVM_VERSION "21.1.8")
set(ISLE_LLVM_SOURCE_ROOT "${ISLE_LLVM_ROOT}/source")
set(ISLE_LLVM_BUILD_ROOT "${ISLE_LLVM_ROOT}/build")
set(ISLE_LLVM_PROVENANCE "${ISLE_LLVM_ROOT}/provenance.json")
set(ISLE_LLVM_NOTICES "${ISLE_LLVM_ROOT}/notices")
set(ISLE_LLVM_LICENSE_FILE "${ISLE_LLVM_SOURCE_ROOT}/llvm/LICENSE.TXT")
if(DEFINED LLVM_DIR AND NOT "${LLVM_DIR}" STREQUAL "" AND
        NOT "${LLVM_DIR}" STREQUAL "${ISLE_LLVM_BUILD_ROOT}/lib/cmake/llvm")
    message(FATAL_ERROR "LLVM_DIR must not override the verified source build; use ISLE_LLVM_ROOT")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/llvm-dependencies.json"
    "${CMAKE_CURRENT_LIST_DIR}/../tools/prepare_llvm.py"
    "${ISLE_LLVM_PROVENANCE}" "${ISLE_LLVM_BUILD_ROOT}/CMakeCache.txt")
execute_process(COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_LIST_DIR}/../tools/prepare_llvm.py"
    --root "${ISLE_LLVM_ROOT}" --verify-sealed
    RESULT_VARIABLE _isle_llvm_verify_result
    OUTPUT_VARIABLE _isle_llvm_verify_output
    ERROR_VARIABLE _isle_llvm_verify_error)
if(NOT _isle_llvm_verify_result EQUAL 0)
    message(FATAL_ERROR "Verified source-backed LLVM is not ready. Explicitly run tools/prepare_llvm.py --root <root> --prepare, then --configure with a compatible compiler, then cmake --build <root>/build --config Release --parallel 2 --target LLVMOrcJIT LLVMX86CodeGen LLVMX86AsmParser LLVMX86Desc LLVMX86Info LLVMCore LLVMExecutionEngine LLVMRuntimeDyld LLVMPasses, then tools/prepare_llvm.py --root <root> --seal-build. Pass -DISLE_LLVM_ROOT=<root>. Use a matching Debug configuration for Windows Debug /MTd. No project configure downloads or compiles LLVM.\n${_isle_llvm_verify_output}\n${_isle_llvm_verify_error}")
endif()
file(READ "${ISLE_LLVM_PROVENANCE}" _isle_llvm_provenance)
foreach(_field IN ITEMS compiler_id compiler_version compiler_sha256 system processor pointer_size build_type msvc_runtime standard_library)
    string(JSON _isle_llvm_${_field} GET "${_isle_llvm_provenance}" build ${_field})
endforeach()
if(NOT _isle_llvm_system STREQUAL CMAKE_SYSTEM_NAME OR
        NOT _isle_llvm_pointer_size STREQUAL "8")
    message(FATAL_ERROR "LLVM build platform differs from the native project platform")
endif()
if(WIN32)
    if(NOT MSVC OR NOT CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "x64")
        message(FATAL_ERROR "LLVM Windows production build requires native MSVC x64")
    endif()
    file(SHA256 "${CMAKE_CXX_COMPILER}" _isle_project_compiler_sha256)
    if(NOT _isle_llvm_compiler_id STREQUAL "MSVC" OR
            NOT _isle_llvm_compiler_version VERSION_EQUAL CMAKE_CXX_COMPILER_VERSION OR
            NOT _isle_llvm_compiler_sha256 STREQUAL _isle_project_compiler_sha256)
        message(FATAL_ERROR "Windows LLVM and project must use the same native MSVC toolchain")
    endif()
    if(CMAKE_CONFIGURATION_TYPES)
        list(LENGTH CMAKE_CONFIGURATION_TYPES _isle_llvm_config_count)
        if(NOT _isle_llvm_config_count EQUAL 1)
            message(FATAL_ERROR "Select one matching LLVM CRT configuration using -DCMAKE_CONFIGURATION_TYPES=Release (or Debug)")
        endif()
        set(_isle_project_config "${CMAKE_CONFIGURATION_TYPES}")
    else()
        set(_isle_project_config "${CMAKE_BUILD_TYPE}")
    endif()
    if(NOT _isle_project_config STREQUAL _isle_llvm_build_type OR
            NOT CMAKE_MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        message(FATAL_ERROR "Project and LLVM must use matching native static /MT or /MTd configuration")
    endif()
else()
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _isle_llvm_processor)
    if(NOT _isle_llvm_processor MATCHES "^(x86_64|amd64|x64)$")
        message(FATAL_ERROR "LLVM production dependency supports only native X86-64")
    endif()
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$" OR
            NOT _isle_llvm_compiler_id MATCHES "^(GNU|Clang)$" OR
            NOT _isle_llvm_standard_library STREQUAL "libstdc++" OR
            NOT "stdc++" IN_LIST CMAKE_CXX_IMPLICIT_LINK_LIBRARIES)
        message(FATAL_ERROR "Linux LLVM reuse requires native GNU/Clang with the same libstdc++ ABI")
    endif()
    get_cmake_property(_isle_project_variables VARIABLES)
    foreach(_flags IN LISTS _isle_project_variables)
        if(NOT _flags MATCHES "^CMAKE_CXX_FLAGS($|_)")
            continue()
        endif()
        if("${${_flags}}" MATCHES "_GLIBCXX_USE_CXX11_ABI[ \t]*=[ \t]*0|_GLIBCXX_DEBUG|_GLIBCXX_PROFILE|-stdlib=libc\\+\\+|-fabi-version")
            message(FATAL_ERROR "Project C++ ABI override is incompatible with the verified native LLVM build")
        endif()
    endforeach()
endif()

# Restrict discovery to the verified native build tree; never use system/prebuilt
# LLVM or hard-coded guesses about component library filenames/transitive deps.
set(LLVM_DIR "${ISLE_LLVM_BUILD_ROOT}/lib/cmake/llvm")
find_package(LLVM ${ISLE_LLVM_VERSION} EXACT REQUIRED CONFIG
    PATHS "${LLVM_DIR}" NO_DEFAULT_PATH)
if(NOT LLVM_TARGETS_TO_BUILD STREQUAL "X86" OR LLVM_BUILD_LLVM_DYLIB OR LLVM_LINK_LLVM_DYLIB)
    message(FATAL_ERROR "LLVM package is not the selected static X86-only source build")
endif()
# nativecodegen expands only CodeGen/Desc/Info. The broader native pseudo
# component also selects the unused X86 parser/disassembler, which are not
# required by InitializeNativeTarget/InitializeNativeTargetAsmPrinter.
llvm_map_components_to_libnames(ISLE_LLVM_LIBRARIES
    OrcJIT ExecutionEngine RuntimeDyld Object nativecodegen
    Core Support Passes)
foreach(_library IN LISTS ISLE_LLVM_LIBRARIES)
    if(NOT TARGET ${_library})
        message(FATAL_ERROR "Verified LLVM package is missing component target ${_library}")
    endif()
    get_target_property(_isle_llvm_library_type ${_library} TYPE)
    if(NOT _isle_llvm_library_type STREQUAL "STATIC_LIBRARY")
        message(FATAL_ERROR "LLVM component ${_library} is not static")
    endif()
endforeach()
add_library(isle-llvm INTERFACE)
target_include_directories(isle-llvm SYSTEM INTERFACE ${LLVM_INCLUDE_DIRS})
separate_arguments(_isle_llvm_definitions NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_options(isle-llvm INTERFACE ${_isle_llvm_definitions})
target_link_libraries(isle-llvm INTERFACE ${ISLE_LLVM_LIBRARIES})
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Hide ONLY statically linked LLVM archives from the executable dynamic
    # symbol table. Mesa may legitimately load its own system libLLVM; it must
    # not bind against this application's independent LLVM ORC implementation.
    set(_isle_llvm_hidden_archives "")
    foreach(_library IN LISTS LLVM_AVAILABLE_LIBS)
        if(TARGET ${_library})
            get_target_property(_type ${_library} TYPE)
            if(_type STREQUAL "STATIC_LIBRARY")
                list(APPEND _isle_llvm_hidden_archives "$<TARGET_FILE_NAME:${_library}>")
            endif()
        endif()
    endforeach()
    list(JOIN _isle_llvm_hidden_archives ":" _isle_llvm_hidden_archives)
    target_link_options(isle-llvm INTERFACE "LINKER:--exclude-libs,${_isle_llvm_hidden_archives}")
endif()
