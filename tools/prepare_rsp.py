#!/usr/bin/env python3
"""Acquire the exact private RSP/JIT snapshot, adapt it, and retain all notices."""
#
# Original tooling: MIT, copyright 2026 35mmIsle contributors.
# MIT-selected parallel-rsp matching excerpts: copyright (c) 2020
# Hans-Kristian Arntzen; permission and warranty terms in the root LICENSE.
# GNU lightning is not relicensed: its original bodies/notices and our native
# adaptations remain in the private LGPL-3.0-or-later source composition.

import argparse
import hashlib
import json
from pathlib import Path
import tarfile
import tempfile

from prepare_rdp import acquire, unpack, publish_tree, copy_file

PIN = ('parallel-rsp', 'libretro/parallel-rsp',
       '9ab776a2ce28899769c4f911403234cac62fc799',
       'e03ad7e3277a9f763aab0aff86da350cf93fecb66ef7f616298d84755fa6275d')
INPUT_HASHES = {
    'state.hpp': '40728a07b19bd0d6693acaa59e1616780906c0ef5199c69e19f830aee26bf0f8',
    'rsp_jit.cpp': '7d32fbc17e576fa08c157c9dac51f3ba27de1674423d13219db64d96703f6793',
    'lightning/include/lightning.h': 'e4ad93cf2ee41774cb0f98d82314ff935eafc7b0577fca2d5d55704cdd906c65',
    'lightning/lib/jit_x86.c': '4435d548b390948f2ff8d5a8a88dd7e27d7daaa5ce3ff7cd39a9c17200947210',
    'lightning/lib/jit_x86-cpu.c': '61faae8b73208a14b392c4ac98765ebd4bbc809495b9b8be016a7b8f394c3dee',
    'arch/simd/rsp/rsp_core.cpp': '43cacdf5c8170afa6d55563b1012508740061159fe348eaa2447173dfb98b32d',
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def replace_once(text, before, after):
    if text.count(before) != 1:
        raise ValueError('exact private patch anchor is missing or ambiguous: ' + before[:80])
    return text.replace(before, after, 1)


def adapt(relative, text):
    if relative == 'state.hpp':
        text = replace_once(text, '\tCP0 cp0;\n', '''\tCP0 cp0;
	// Private host-service and bounded-basic-block integration. Not emulated cycles.
	uint32_t block_budget = 0;
	uint32_t service_register = 0;
	uint32_t service_value = 0;
''')
        text = replace_once(text, '\tMODE_CHECK_FLAGS = 4\n', '''\tMODE_CHECK_FLAGS = 4,
	MODE_HOST_SERVICE = 5,
	MODE_BUDGET = 6,
	MODE_HOST_ERROR = 7
''')
    elif relative == 'rsp_jit.cpp':
        # Foreign generated machine code has no Clang C++ function-type cookie.
        # Keep ASan and every other UBSan check on this boundary and its callers.
        text = replace_once(text, 'int CPU::enter(uint32_t pc)',
            '#if defined(__clang__)\n__attribute__((no_sanitize("function")))\n#endif\nint CPU::enter(uint32_t pc)')
        text = replace_once(text, 'regs.immediate_mips_register(_jit, rt, imm << 16);',
                            'regs.immediate_mips_register(_jit, rt, int32_t(imm) * 65536);')
        text = replace_once(text, '\tinit_jit("RSP");\n', '''	// Lightning owns process-global feature tables. Keep them alive across CPUs.
	struct LightningLifetime
	{
		LightningLifetime() { init_jit("RSP"); }
		~LightningLifetime() { finish_jit(); }
	};
	static LightningLifetime lifetime;
''')
        text = replace_once(text, '\tfinish_jit();\n', '')
        text = replace_once(text, '''	for (unsigned i = 0; i < CODE_BLOCKS; i++)
		if (memcmp(cached_imem + i * CODE_BLOCK_WORDS, state.imem + i * CODE_BLOCK_WORDS, CODE_BLOCK_SIZE))
			state.dirty_blocks |= (0x3 << i) >> 1;
''', '''	// DMA reloads may overlap either half of a compiled region. Flush every entry.
	state.dirty_blocks = ~0u;
''')
        text = replace_once(text, '''	static Func rsp_enter(void *cpu, unsigned pc)
	{
		return static_cast<CPU *>(cpu)->get_jit_block(pc);
	}
''', '''	static Func rsp_enter(void *cpu, unsigned pc) noexcept
	{
		// A block lookup/compilation can allocate. Never unwind through JIT frames.
		try { return static_cast<CPU *>(cpu)->get_jit_block(pc); }
		catch (...) { return nullptr; }
	}
''')
        text = replace_once(text, '''	jit_prepare();
	jit_pushargr(JIT_REGISTER_STATE);
	jit_pushargr(JIT_REGISTER_NEXT_PC);
	jit_finishi(reinterpret_cast<jit_pointer_t>(rsp_enter));
	jit_retval(JIT_REGISTER_NEXT_PC);
''', '''	jit_stxi_i(offsetof(CPUState, pc), JIT_REGISTER_STATE, JIT_REGISTER_NEXT_PC);
	jit_prepare();
	jit_pushargr(JIT_REGISTER_STATE);
	jit_pushargr(JIT_REGISTER_NEXT_PC);
	jit_finishi(reinterpret_cast<jit_pointer_t>(rsp_enter));
	jit_retval(JIT_REGISTER_NEXT_PC);
	jit_node_t *resolved_block = jit_bnei(JIT_REGISTER_NEXT_PC, 0);
	jit_ldxi_i(JIT_REGISTER_NEXT_PC, JIT_REGISTER_STATE, offsetof(CPUState, pc));
	jit_movi(JIT_REGISTER_MODE, MODE_HOST_ERROR);
	jit_node_t *failed_lookup = jit_jmpi();
	jit_patch(resolved_block);
''')
        text = replace_once(text, '\tauto *return_label = jit_indirect();\n', '''	auto *return_label = jit_indirect();
	jit_patch_at(failed_lookup, return_label);
''')
        text = replace_once(text, '''		if (block_entry[i])
		{
			// Before we enter into a new block, we have to flush register window since someone can branch here.
			regs.flush_register_window(_jit);
			regs.reset();
			branch_targets[i] = jit_label();
		}
''', '''		// Guard every external entry, local branch destination, and fallthrough
		// following a branch's delay slot. Local gotos land before this guard.
		const bool fallthrough_entry = i >= 2 &&
		    (((state.imem[pc_word + i - 2] >> 26) >= 1 &&
		      (state.imem[pc_word + i - 2] >> 26) <= 7) ||
		     ((state.imem[pc_word + i - 2] & 0xfc00003eu) == 8));
		if (i == 0 || block_entry[i] || fallthrough_entry)
		{
			regs.flush_register_window(_jit);
			regs.reset();
			if (block_entry[i])
				branch_targets[i] = jit_label();
			jit_ldxi_ui(JIT_REGISTER_NEXT_PC, JIT_REGISTER_STATE, offsetof(CPUState, block_budget));
			jit_node_t *within_budget = jit_bnei(JIT_REGISTER_NEXT_PC, 0);
			jit_movi(JIT_REGISTER_NEXT_PC, (pc_word + i) << 2);
			jit_movi(JIT_REGISTER_MODE, MODE_BUDGET);
			jit_patch_abs(jit_jmpi(), thunks.return_thunk);
			jit_patch(within_budget);
			jit_subi(JIT_REGISTER_NEXT_PC, JIT_REGISTER_NEXT_PC, 1);
			jit_stxi_i(offsetof(CPUState, block_budget), JIT_REGISTER_STATE, JIT_REGISTER_NEXT_PC);
		}
''')
        text = replace_once(text, '''#ifndef PARALLEL_INTEGRATION
			print_registers();
#endif
''', '')
        text = replace_once(text, '''		case MODE_CHECK_FLAGS:
		case MODE_DMA_READ:
''', '''		case MODE_CHECK_FLAGS:
		case MODE_DMA_READ:
		case MODE_HOST_SERVICE:
		case MODE_BUDGET:
		case MODE_HOST_ERROR:
''')
    elif relative == 'lightning/lib/jit_x86-cpu.c':
        # x86 instruction immediates/relocations are byte-aligned, not naturally
        # aligned C objects. memcpy preserves exact instruction bytes without UB.
        text = replace_once(text, '#  define is(s)\t\t\t\t*_jit->pc.us++ = s',
            '#  define is(s) do { jit_uint16_t v_ = (jit_uint16_t)(s); memcpy(_jit->pc.uc, &v_, sizeof(v_)); _jit->pc.uc += sizeof(v_); } while (0)')
        text = replace_once(text, '#  define ii(i)\t\t\t\t*_jit->pc.ui++ = i',
            '#  define ii(i) do { jit_uint32_t v_ = (jit_uint32_t)(i); memcpy(_jit->pc.uc, &v_, sizeof(v_)); _jit->pc.uc += sizeof(v_); } while (0)')
        text = replace_once(text, '#    define il(l)\t\t\t*_jit->pc.ul++ = l',
            '#    define il(l) do { jit_uint64_t v_ = (jit_uint64_t)(l); memcpy(_jit->pc.uc, &v_, sizeof(v_)); _jit->pc.uc += sizeof(v_); } while (0)')
        text = replace_once(text, '*(jit_word_t *)instr = label;',
                            'memcpy((void *)instr, &label, sizeof(label));')
        text = replace_once(text, '*(jit_int32_t *)instr = disp;',
                            '{ jit_int32_t value = (jit_int32_t)disp; memcpy((void *)instr, &value, sizeof(value)); }')
        # Standard C case labels are equivalent and compile under native MSVC.
        for first, last in ((0xb8, 0xbf), (0x70, 0x7f)):
            labels = ' '.join('case 0x%02x:' % value for value in range(first, last + 1))
            text = replace_once(text, 'case 0x%02x ... 0x%02x:' % (first, last), labels)
    elif relative == 'lightning/include/lightning.h':
        text = replace_once(text, '#include <unistd.h>\n', '''#if !defined(_MSC_VER)
#include <unistd.h>
#endif
''')
        text = replace_once(text, '#include <pthread.h>\n', '''#if defined(_MSC_VER)
#include <windows.h>
#include <intrin.h>
/* Original private MSVC adaptations for the bundled LGPL lightning backend. */
#ifndef __x86_64__
#define __x86_64__ 1
#endif
#define HAVE_FFSL 1
static inline int isle_lightning_ffsl(unsigned long long value)
{
    unsigned long index;
    return _BitScanForward64(&index, value) ? (int)index + 1 : 0;
}
#define ffsl(value) isle_lightning_ffsl((unsigned long long)(value))
typedef SRWLOCK pthread_mutex_t;
#define PTHREAD_MUTEX_INITIALIZER SRWLOCK_INIT
static inline int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    AcquireSRWLockExclusive(mutex);
    return 0;
}
static inline int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    ReleaseSRWLockExclusive(mutex);
    return 0;
}
#else
#include <pthread.h>
#endif
''')
    elif relative == 'lightning/lib/jit_x86.c':
        # Whole-file SHA256 above fixes these original line spans. Read GNU
        # assembler bodies from the private input instead of embedding them in
        # this public tool; only the original MSVC replacements live here.
        lines = text.splitlines(keepends=True)
        for first, last, leaf in ((457, 464, '(int)0x80000001u'),
                                  (448, 450, '7'), (419, 426, '1')):
            original = ''.join(lines[first - 1:last])
            native = ('#if defined(_MSC_VER)\n'
                '    { int values[4]; __cpuidex(values, %s, 0);\n'
                '      eax = (jit_uint32_t)values[0]; ebx.cpuid = (jit_uint32_t)values[1];\n'
                '      ecx.cpuid = (jit_uint32_t)values[2]; edx.cpuid = (jit_uint32_t)values[3]; }\n'
                '#else\n') % leaf
            lines[first - 1:last] = [native, original, '#endif\n']
        text = ''.join(lines)
    elif relative == 'arch/simd/rsp/rsp_core.cpp':
        text = replace_once(text, '#include <string.h>\n', '''#include <string.h>
#if defined(_MSC_VER)
#include <intrin.h>
#define __builtin_bswap64(value) _byteswap_uint64(value)
#endif
''')
        # An output-only GNU asm constraint supplied arbitrary initial bits.
        # These cases overwrite every subsequently observed lane; zero has the
        # same result and uses the native MSVC intrinsic instead of GNU asm.
        before = '\t\t__asm__("" : "=x"(v)); /* Do not remove. */'
        if text.count(before) != 2:
            raise ValueError('unexpected exact SIMD initializer anchors')
        text = text.replace(before, '''#if defined(_MSC_VER)
        v = _mm_setzero_si128();
#else
''' + before + '\n#endif')
        text = replace_once(text, '''#ifdef NDEBUG
	__builtin_unreachable();
#else
	__builtin_trap();
#endif''', '''#if defined(_MSC_VER)
    __debugbreak();
    __assume(0);
#elif defined(NDEBUG)
	__builtin_unreachable();
#else
	__builtin_trap();
#endif''')
    return text


def prepare(manifest_path, output):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    records = manifest['dependencies']
    if manifest['schema_version'] != 1 or len(records) != 1:
        raise ValueError('unsupported RSP dependency manifest')
    record = records[0]
    if tuple(record[key] for key in ('name', 'repository', 'commit', 'archive_sha256')) != PIN:
        raise ValueError('RSP identity differs from the exact supported pin')
    output.mkdir(parents=True, exist_ok=True)
    cache = output / 'archives'
    cache.mkdir(exist_ok=True)
    adaptations = []
    with tempfile.TemporaryDirectory(prefix='staging-', dir=output) as temporary:
        staging = Path(temporary)
        source = staging / 'source'
        source.mkdir()
        unpack(acquire(record, cache), source)
        # Keep the complete unmodified archive in archives and every source
        # notice, even files not selected by our library-only CMake composition.
        notices = staging / 'notices'
        for path in source.rglob('*'):
            relative = path.relative_to(source)
            lower = path.name.lower()
            in_license_directory = any(part.lower() in ('license', 'licenses') for part in relative.parts[:-1])
            if path.is_file() and (in_license_directory or lower.startswith(('license', 'copying', 'copyright', 'notice', 'credits')) or lower.endswith('.license')):
                copy_file(path, notices / relative)
        for relative, expected in INPUT_HASHES.items():
            path = source / relative
            original = path.read_bytes()
            if digest(original) != expected:
                raise ValueError('unexpected exact patch input: ' + relative)
            adapted = adapt(relative, original.decode('utf-8')).encode('utf-8')
            path.write_bytes(adapted)
            adaptations.append({'path': relative, 'original_sha256': expected,
                                'adapted_sha256': digest(adapted)})
        publish_tree(source, output / 'source')
        publish_tree(notices, output / 'notices')
    evidence = {'schema_version': 1, 'manifest_sha256': digest(manifest_bytes),
                'generator_sha256': digest(Path(__file__).read_bytes()),
                'helper_sha256': digest(Path(__file__).with_name('prepare_rdp.py').read_bytes()),
                'dependencies': records, 'adaptations': adaptations,
                'scope': manifest['scope'],
                'patch_identity': 'bounded-block-host-services-msvc-safe-emission-v2',
                'composition': 'Library-only explicit source list; upstream CP0, runners, fuzzer, debug JIT, GNU computed-jump tracing, and app not compiled',
                'license_notice': 'Core MIT selection does not cover statically linked GNU lightning (LGPL-3.0-or-later) or other bundled terms; all notices retained. Private experiment only; no release approval.'}
    (output / 'provenance.json').write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')
    print('Prepared exact private RSP/JIT sources and complete notices at ' + str(output))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        prepare(args.manifest, args.output)
    except (OSError, ValueError, KeyError, TypeError, tarfile.TarError) as error:
        parser.exit(1, 'Pinned RSP preparation failed: %s\n' % error)


if __name__ == '__main__':
    main()
