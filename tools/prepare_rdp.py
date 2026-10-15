#!/usr/bin/env python3
"""Acquire digest-checked immutable RDP inputs and materialize a private permissive subset."""

import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import shutil
import tarfile
import tempfile
import urllib.request


# This isolated project builds the real pinned Granite slangmosh, including its
# shaderc/glslang/SPIRV-Tools compiler. No PATH-provided shader binary is used.
TOOL_CMAKE = r'''cmake_minimum_required(VERSION 3.24)
project(IsleRdpShaderCompiler LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# These are archive snapshots, not the containing 35mmIsle Git checkout.
set(ENV{GIT_CEILING_DIRECTORIES} "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(Threads REQUIRED)
function(add_granite_internal_lib name)
    add_library(${name} STATIC ${ARGN})
    target_link_libraries(${name} PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endfunction()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_INSTALL ON CACHE BOOL "" FORCE)
set(SHADERC_ENABLE_SHARED_CRT ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE BOOL "" FORCE)
set(GLSLANG_TESTS OFF CACHE BOOL "" FORCE)
set(SHADERC_ENABLE_WGSL_OUTPUT OFF CACHE BOOL "" FORCE)
set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "" FORCE)
set(SPIRV_HEADERS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL ON CACHE BOOL "" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
set(SPIRV_CHECK_CONTEXT OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_GLSL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "" FORCE)
set(SHADERC_THIRD_PARTY_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party" CACHE STRING "" FORCE)
set(SPIRV-Headers_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/spirv-headers" CACHE STRING "" FORCE)
add_subdirectory(third_party/shaderc EXCLUDE_FROM_ALL)
add_subdirectory(third_party/spirv-cross EXCLUDE_FROM_ALL)
add_library(granite-rapidjson INTERFACE)
target_include_directories(granite-rapidjson INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/third_party" "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidjson/include")
add_library(granite-volk-headers INTERFACE)
target_include_directories(granite-volk-headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/volk" "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vulkan-headers/include")
add_library(granite-volk STATIC third_party/volk/volk.c)
target_link_libraries(granite-volk PUBLIC granite-volk-headers PRIVATE ${CMAKE_DL_LIBS})
if(WIN32)
    target_compile_definitions(granite-volk PUBLIC VK_USE_PLATFORM_WIN32_KHR)
endif()
set(GRANITE_SHADER_COMPILER_OPTIMIZE ON)
set(GRANITE_VULKAN_SPIRV_CROSS ON)
set(GRANITE_VULKAN_SYSTEM_HANDLES OFF)
set(GRANITE_RENDERDOC_CAPTURE OFF)
set(GRANITE_VULKAN_DXGI_INTEROP OFF)
set(GRANITE_SHIPPING ON)
foreach(part util path threading compiler filesystem event application/global vulkan)
    add_subdirectory(Granite/${part})
endforeach()
add_executable(slangmosh Granite/slangmosh/slangmosh.cpp)
target_link_libraries(slangmosh PRIVATE granite-vulkan granite-compiler granite-filesystem
    granite-path granite-application-global-init granite-threading granite-rapidjson)
set_target_properties(slangmosh PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(slangmosh PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/bin")
endforeach()
'''


def digest(data):
    return hashlib.sha256(data).hexdigest()


def acquire(record, cache):
    filename = cache / (record['name'] + '-' + record['commit'] + '.tar.gz')
    if filename.is_file():
        data = filename.read_bytes()
    else:
        url = 'https://codeload.github.com/{}/tar.gz/{}'.format(record['repository'], record['commit'])
        with urllib.request.urlopen(url, timeout=120) as response:
            data = response.read()
        if digest(data) != record['archive_sha256']:
            raise ValueError('download SHA-256 mismatch: ' + record['name'])
        filename.write_bytes(data)
    if digest(data) != record['archive_sha256']:
        raise ValueError('cached archive SHA-256 mismatch (remove this corrupt archive): ' + str(filename))
    return data


def unpack(data, destination):
    # Deliberately accept regular files/directories only; no archive path can
    # escape the private staging directory and no links can import other bytes.
    with tarfile.open(fileobj=io.BytesIO(data), mode='r:gz') as archive:
        roots = set()
        for member in archive.getmembers():
            path = PurePosixPath(member.name)
            if path.is_absolute() or '..' in path.parts or not path.parts:
                raise ValueError('unsafe archive member: ' + member.name)
            roots.add(path.parts[0])
            target = destination.joinpath(*path.parts[1:])
            if member.isdir():
                target.mkdir(parents=True, exist_ok=True)
            elif member.isfile():
                target.parent.mkdir(parents=True, exist_ok=True)
                with archive.extractfile(member) as source, target.open('wb') as output:
                    shutil.copyfileobj(source, output)
            else:
                raise ValueError('unsupported archive link/special member: ' + member.name)
        if len(roots) != 1:
            raise ValueError('archive must have exactly one source root')


def copy_file(source, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)

def publish_tree(source, target, generated=()):
    # Revalidate every input on configure without forcing an unchanged compiler
    # closure to rebuild. Only declared build outputs may survive outside input.
    target.mkdir(parents=True, exist_ok=True)
    if target.is_symlink():
        raise ValueError('private materialization root must not be a symlink')
    expected = {path.relative_to(source) for path in source.rglob('*')}
    for path in sorted(target.rglob('*'), key=lambda item: len(item.parts), reverse=True):
        if path.is_symlink():
            raise ValueError('unexpected symlink in private materialization: ' + str(path))
        relative = path.relative_to(target)
        if relative not in expected and relative.as_posix() not in generated:
            if path.is_dir():
                path.rmdir()
            else:
                path.unlink()
    for path in source.rglob('*'):
        destination = target / path.relative_to(source)
        if path.is_dir():
            destination.mkdir(parents=True, exist_ok=True)
        elif not destination.is_file() or destination.read_bytes() != path.read_bytes():
            copy_file(path, destination)



def adapt_compiler(compiler):
    # GCC 15 no longer supplies uint32_t through incidental standard includes.
    # Adapt only this exact pinned header in the private compiler snapshot.
    relative = 'third_party/glslang/SPIRV/SpvBuilder.h'
    path = compiler / relative
    original = path.read_bytes()
    original_hash = '9d9c01898c19b3b63d131af6dc1fa4e57e1cdc8be5a59e7d5e3aa40e6f7e0732'
    adapted_hash = 'af38df8079d72bb60672bc52e8658870f4907573f00ccda2700da9206953420f'
    if digest(original) != original_hash:
        raise ValueError('unexpected glslang SpvBuilder.h adaptation input')
    adapted = original.replace(b'#include <algorithm>\n',
                               b'#include <algorithm>\n#include <cstdint>\n', 1)
    if digest(adapted) != adapted_hash:
        raise ValueError('unexpected glslang SpvBuilder.h adaptation output')
    path.write_bytes(adapted)
    return {'path': relative, 'original_sha256': original_hash,
            'adapted_sha256': adapted_hash,
            'change': 'Declare uint32_t through its required cstdint header; no algorithm change'}


def adapt_granite_hash(compiler):
    relative = 'Granite/util/compile_time_hash.hpp'
    path = compiler / relative
    original = path.read_bytes()
    original_hash = '518bcd7edc90774fbc57d5a59ba26f553d696e8ebde496e6f5493e061c71efda'
    adapted_hash = '6f0066c44aec4107ad6e63cf891200d33a13955a5e51b9cb668a98628dca19a5'
    if digest(original) != original_hash:
        raise ValueError('unexpected Granite compile_time_hash.hpp adaptation input')
    adapted = original.replace(b'#pragma once\n',
                               b'#pragma once\n\n#include <cstddef>\n#include <cstdint>\n', 1)
    if digest(adapted) != adapted_hash:
        raise ValueError('unexpected Granite compile_time_hash.hpp adaptation output')
    path.write_bytes(adapted)
    return {'path': relative, 'original_sha256': original_hash,
            'adapted_sha256': adapted_hash,
            'change': 'Declare size_t and integer types through their required headers; no algorithm change'}


def prepare(manifest_path, output):
    manifest_data = manifest_path.read_bytes()
    manifest = json.loads(manifest_data)
    required = {'parallel-rdp', 'Granite', 'volk', 'vulkan-headers', 'shaderc',
                'glslang', 'spirv-tools', 'spirv-headers', 'spirv-cross', 'rapidjson'}
    records = manifest['dependencies']
    if manifest['schema_version'] != 1 or {r['name'] for r in records} != required or len(records) != len(required):
        raise ValueError('unsupported/incomplete RDP dependency manifest')
    for record in records:
        if (re.fullmatch(r'[0-9a-f]{40}', record['commit']) is None or
                re.fullmatch(r'[0-9a-f]{64}', record['archive_sha256']) is None or
                re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', record['repository']) is None):
            raise ValueError('floating/invalid dependency identity')
    output.mkdir(parents=True, exist_ok=True)
    cache = output / 'archives'
    cache.mkdir(exist_ok=True)
    # Always rematerialize committed, digest-checked bytes. Never trust a prior
    # mutable extraction or an arbitrary source-directory override.
    with tempfile.TemporaryDirectory(prefix='staging-', dir=output) as temporary:
        staging = Path(temporary)
        sources = {}
        for record in records:
            root = staging / record['name']
            root.mkdir()
            unpack(acquire(record, cache), root)
            sources[record['name']] = root
        renderer = staging / 'renderer-output'
        rdp = sources['parallel-rdp']
        granite = sources['Granite']
        for pattern in ('*.cpp', '*.hpp', 'shaders/*.comp', 'shaders/*.vert',
                        'shaders/*.frag', 'shaders/*.h', 'shaders/*.json'):
            for source in (rdp / 'parallel-rdp').glob(pattern):
                copy_file(source, renderer / 'parallel-rdp' / source.relative_to(rdp / 'parallel-rdp'))
        copy_file(granite / 'assets/shaders/inc/debug_channel.h',
                  renderer / 'Granite/assets/shaders/inc/debug_channel.h')
        for source in (granite / 'vulkan').iterdir():
            if source.suffix in ('.cpp', '.hpp') and not source.name.startswith('wsi_dxgi.'):
                copy_file(source, renderer / 'vulkan' / source.name)
        for source in (granite / 'vulkan' / 'texture').glob('texture_format.*'):
            copy_file(source, renderer / 'vulkan' / 'texture' / source.name)
        util = ('aligned_alloc', 'arena_allocator', 'timer', 'logging', 'timeline_trace_file',
                'thread_name', 'thread_id', 'environment')
        for name in util:
            for suffix in ('.cpp', '.hpp'):
                copy_file(granite / 'util' / (name + suffix), renderer / 'util' / (name + suffix))
        for name in ('bitops', 'enum_cast', 'hash', 'intrusive', 'intrusive_hash_map',
                     'intrusive_list', 'object_pool', 'read_write_lock', 'small_vector',
                     'stack_allocator', 'temporary_hashmap', 'dynamic_array'):
            copy_file(granite / 'util' / (name + '.hpp'), renderer / 'util' / (name + '.hpp'))
        for name in ('volk.c', 'volk.h'):
            copy_file(sources['volk'] / name, renderer / 'volk' / name)
        shutil.copytree(sources['vulkan-headers'] / 'include', renderer / 'vulkan-headers' / 'include')
        copy_file(rdp / 'LICENSE', renderer / 'LICENSE')
        copy_file(rdp / 'parallel-rdp' / 'config.mk', renderer / 'config.mk')
        compiler = staging / 'compiler-output'
        for part in ('util', 'path', 'threading', 'compiler', 'filesystem', 'event',
                     'application/global', 'vulkan', 'slangmosh'):
            shutil.copytree(granite / part, compiler / 'Granite' / part)
        copy_file(granite / 'LICENSE', compiler / 'Granite' / 'LICENSE')
        for name in required - {'parallel-rdp', 'Granite'}:
            shutil.copytree(sources[name], compiler / 'third_party' / name)
        (compiler / 'CMakeLists.txt').write_text(TOOL_CMAKE, encoding='utf-8')
        copy_file(granite / 'third_party' / 'rapidjson_wrapper.hpp', compiler / 'third_party' / 'rapidjson_wrapper.hpp')
        adaptations = [adapt_compiler(compiler), adapt_granite_hash(compiler)]
        notices = staging / 'notices-output'
        for name, root in sources.items():
            for source in root.rglob('*'):
                lower = source.name.lower()
                in_license_directory = any(part.lower() in ('licenses', 'license') for part in source.relative_to(root).parts[:-1])
                if source.is_file() and (in_license_directory or lower.startswith(('license', 'copying', 'copyright', 'notice')) or lower.endswith('.license')):
                    copy_file(source, notices / name / source.relative_to(root))
        # Header-embedded notices stay verbatim in every selected source file.
        # These complete upstream notices accompany both runtime and build tools.
        for name, source in (('renderer', renderer), ('compiler', compiler), ('notices', notices)):
            generated = ('parallel-rdp/shaders/slangmosh.hpp',) if name == 'renderer' else ()
            publish_tree(source, output / name, generated)
    evidence = {'schema_version': 1, 'manifest_sha256': digest(manifest_data),
                'generator_sha256': digest(Path(__file__).read_bytes()),
                'dependencies': records, 'adaptations': adaptations,
                'shader_command': 'slangmosh --strip -O --namespace RDP parallel-rdp/shaders/slangmosh.json --output parallel-rdp/shaders/slangmosh.hpp',
                'scope': manifest['scope'], 'runtime_system_inputs': 'Vulkan loader and ICD/driver are external, reported at execution; no validation/performance claim from preparation'}
    (output / 'provenance.json').write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')
    print('Prepared exact renderer subset, compiler closure and notices at ' + str(output))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        prepare(args.manifest, args.output)
    except (OSError, ValueError, KeyError, TypeError, tarfile.TarError) as error:
        parser.exit(1, 'Pinned RDP preparation failed: %s\n' % error)


if __name__ == '__main__':
    main()
