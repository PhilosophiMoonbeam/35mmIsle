#!/usr/bin/env python3
"""Real native loader boundary tests; pass the built audit target explicitly.

Run: python3 tests/runtime_custody_tests.py --audit-module /absolute/audit.so
Fixtures are our own C sources, compiled into TemporaryDirectory. No ROM, SDK,
source-text assertions, simulated loader, or mocked admission is involved.
"""
import argparse
import errno
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import time
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import runtime_custody as custody
from runtime_relink import elf_identity

AUDIT_MODULE = None
COMPILER = None

APPLICATION = r'''
#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <link.h>
#include <pthread.h>
#include <linux/sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <sys/syscall.h>
#include <unistd.h>
struct mapping_work { const char *path; int error; };
static int map_file(const char *path, int direct) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 12;
    void *mapped = mmap(0, 4096, PROT_READ | (direct ? PROT_EXEC : 0), MAP_PRIVATE, fd, 0);
    close(fd);
    if (mapped == MAP_FAILED) return 13;
    if (!direct && mprotect(mapped, 4096, PROT_READ | PROT_EXEC)) {
        munmap(mapped, 4096);
        return 14;
    }
    unsigned char first = *(volatile const unsigned char *)mapped;
    if (munmap(mapped, 4096)) return 15;
    return first == 0x7f ? 0 : 16;
}
static void *map_worker(void *raw) {
    struct mapping_work *work = raw;
    for (int index = 0; index < 8 && !work->error; ++index)
        work->error = map_file(work->path, 0);
    return NULL;
}
static void *leaderless_worker(void *raw) {
    usleep(30000);
    const char *path = raw;
    int result = map_file(path, 0);
    void *handle = result ? NULL : dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) _exit(24);
    int (*answer)(void) = (int (*)(void))dlsym(handle, "answer");
    result = answer ? answer() : 0;
    dlclose(handle);
    if (result != 42) _exit(25);
    puts("native-leaderless-work-complete");
    fflush(stdout);
    return NULL;
}
int main(int argc, char **argv) {
    if (argc < 2) return 3;
    fprintf(stderr, "native-fixture-stderr\n");
    if (!strcmp(argv[1], "zero")) { puts("native-zero"); return 0; }
    if (!strcmp(argv[1], "exit7")) return 7;
    if (!strcmp(argv[1], "signal")) { raise(SIGTERM); return 4; }
    if (!strcmp(argv[1], "sleep")) { sleep(10); return 0; }
    if (!strcmp(argv[1], "escaped-clone3") || !strcmp(argv[1], "escaped-clone") ||
        !strcmp(argv[1], "escaped-doublefork")) {
        struct clone_args arguments = {0};
        arguments.flags = CLONE_UNTRACED | CLONE_VFORK | CLONE_PARENT;
        arguments.exit_signal = SIGCHLD;
        long child = !strcmp(argv[1], "escaped-clone3")
            ? syscall(SYS_clone3, &arguments, sizeof(arguments))
            : syscall(SYS_clone, arguments.flags | SIGCHLD, 0, 0, 0, 0);
        if (child < 0) return 77;
        if (!child) {
            if (setsid() < 0) _exit(26);
            if (!strcmp(argv[1], "escaped-doublefork")) {
                pid_t descendant = fork();
                if (descendant < 0) _exit(29);
                if (descendant) _exit(0);
            }
            printf("native-escaped-child=%ld\n", (long)getpid());
            fflush(stdout);
            close(198); close(199); close(0); close(1); close(2);
            sleep(60);
            _exit(0);
        }
        return 0;
    }
    if (!strcmp(argv[1], "read-implies-exec")) {
        personality(READ_IMPLIES_EXEC);
        puts("native-abi-attempt-returned");
        return 0;
    }
    if (!strcmp(argv[1], "x32")) {
        syscall(0x40000000UL | SYS_getpid);
        puts("native-abi-attempt-returned");
        return 0;
    }
    if (!strcmp(argv[1], "fork")) {
        pid_t child = fork();
        if (child < 0) return 17;
        if (!child) { puts("native-child-ran"); fflush(stdout); _exit(0); }
        int status;
        return waitpid(child, &status, 0) == child && WIFEXITED(status) ? 0 : 18;
    }
    if (!strcmp(argv[1], "exec")) {
        execl(argv[0], argv[0], "zero", (char *)NULL);
        return 19;
    }
    if (argc < 3) return 5;
    if (!strcmp(argv[1], "leaderless")) {
        pthread_t worker;
        if (pthread_create(&worker, NULL, leaderless_worker, argv[2])) return 27;
        syscall(SYS_exit, 0);
        return 28;
    }
    if (!strcmp(argv[1], "transient-exec") || !strcmp(argv[1], "transient-protect")) {
        int result = map_file(argv[2], !strcmp(argv[1], "transient-exec"));
        if (!result) puts("native-mapping-complete");
        return result;
    }
    if (!strcmp(argv[1], "threads")) {
        pthread_t threads[2];
        struct mapping_work work[2] = {{argv[2], 0}, {argv[2], 0}};
        if (pthread_create(&threads[0], NULL, map_worker, &work[0])) return 20;
        if (pthread_create(&threads[1], NULL, map_worker, &work[1])) return 21;
        if (pthread_join(threads[0], NULL) || pthread_join(threads[1], NULL)) return 22;
        if (work[0].error || work[1].error) return 23;
        puts("native-thread-mappings-complete");
        return 0;
    }
    if (!strcmp(argv[1], "channel")) close(199);
    if (!strcmp(argv[1], "map")) {
        if (argc < 4) return 6;
        int fd = open(argv[3], O_RDONLY);
        struct stat info;
        if (fd < 0 || fstat(fd, &info)) return 7;
        void *mapped = mmap(0, (size_t) info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        if (mapped == MAP_FAILED) return 8;
    }
    void *handle = !strcmp(argv[1], "namespace")
        ? dlmopen(LM_ID_NEWLM, argv[2], RTLD_NOW | RTLD_LOCAL)
        : dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
    if (!handle) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 9; }
    int (*function)(void) = (int (*)(void)) dlsym(handle, argc > 4 ? argv[4] : "answer");
    if (!function) return 10;
    int value = function();
    printf("native-answer=%d\n", value);
    fflush(stdout);
    dlclose(handle);
    return value == 42 || value == 43 ? 0 : 11;
}
'''
GOOD_LIBRARY = r'''
int isle_fixture_bss[8192];
int answer(void) {
    isle_fixture_bss[8191] = 42;
    return isle_fixture_bss[8191];
}
'''
EVIL_LIBRARY = r'''
static void write_marker(void) {
    const char text[] = "UNADMITTED_CONSTRUCTOR\n";
    __asm__ volatile("syscall" : : "a"(1L), "D"(1L), "S"(text),
                     "d"((long) sizeof(text) - 1) : "rcx", "r11", "memory");
}
__attribute__((constructor)) static void initialize(void) { write_marker(); }
int answer(void) { return 99; }
'''
DEPENDENT_LIBRARY = r'''
extern int answer(void);
int dependent(void) { return answer() + 1; }
'''


def file_entry(role, path, aliases):
    path = str(Path(path).absolute())
    data = Path(path).read_bytes()
    return {'role': role, 'path': path, 'expected_sha256': hashlib.sha256(data).hexdigest(),
            'size': len(data), 'aliases': list(dict.fromkeys(aliases))}


class NativeCustodyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if sys.platform != 'linux' or os.uname().machine != 'x86_64':
            raise unittest.SkipTest('custody is explicitly Linux x86-64 only')
        cls.build = tempfile.TemporaryDirectory(prefix='isle-custody-native-build-')
        cls.build_path = Path(cls.build.name)
        def compile_source(name, source, arguments, libraries=()):
            path = cls.build_path / (name + '.c')
            path.write_text(source)
            subprocess.run([COMPILER, *arguments, str(path), *libraries, '-o', str(cls.build_path / name)],
                           check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        compile_source('fixture', APPLICATION, ['-O2', '-fPIE', '-pie', '-Wl,-z,relro,-z,now'], ['-ldl', '-pthread'])
        compile_source('libisle_late.so', GOOD_LIBRARY,
                       ['-O2', '-fPIC', '-shared', '-nostdlib', '-Wl,-soname,libisle_late.so', '-Wl,-z,relro'])
        compile_source('libisle_evil.so', EVIL_LIBRARY,
                       ['-O2', '-fPIC', '-shared', '-nostdlib', '-Wl,-soname,libisle_late.so'])
        # Keep the needed DSO after the translation unit: --as-needed linkers must
        # see the unresolved answer() before deciding whether to retain NEEDED.
        source = cls.build_path / 'dependent.c'
        source.write_text(DEPENDENT_LIBRARY)
        subprocess.run([COMPILER, '-O2', '-fPIC', '-shared', '-nostdlib', str(source),
                        '-L' + str(cls.build_path), '-lisle_late',
                        '-Wl,-soname,libisle_dependent.so', '-o', str(cls.build_path / 'libisle_dependent.so')],
                       check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        cls.loader_path = elf_identity(cls.build_path / 'fixture')['interpreters'][0]
        if not Path(AUDIT_MODULE).is_file():
            raise RuntimeError('the actual built --audit-module is required')

    @classmethod
    def tearDownClass(cls):
        cls.build.cleanup()

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='isle-custody-native-case-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        for name in ('fixture', 'libisle_late.so', 'libisle_evil.so', 'libisle_dependent.so'):
            shutil.copy2(self.build_path / name, self.root / name)
        self.application = self.root / 'fixture'
        self.late = self.root / 'libisle_late.so'
        self.evil = self.root / 'libisle_evil.so'
        self.dependent = self.root / 'libisle_dependent.so'
        self.record = self.root / 'receipt.json'
        self.manifest_path = self.root / 'manifest.json'

    def manifest(self, providers=(), auditor=None):
        auditor = str(Path(auditor or AUDIT_MODULE).absolute())
        loader = elf_identity(self.loader_path)
        loader_aliases = [self.loader_path]
        if loader['dynamic']['soname']:
            loader_aliases.append(loader['dynamic']['soname'])
        images = [file_entry('application', self.application, [str(self.application)]),
                  file_entry('loader', self.loader_path, loader_aliases),
                  file_entry('auditor', auditor, [auditor])]
        for path in providers:
            metadata = elf_identity(path)
            images.append(file_entry('provider', path, [str(path), metadata['dynamic']['soname']]))
        # Resolve only the real fixture compiler's named runtime files. The
        # resulting paths/hashes become explicit manifest entries before launch.
        # This is fixture setup, never a supervisor fallback search/admission.
        aliases = {alias for image in images for alias in image['aliases']}
        cursor = 0
        while cursor < len(images):
            metadata = elf_identity(images[cursor]['path'])
            cursor += 1
            for needed in metadata['dynamic']['needed']:
                if needed in aliases:
                    continue
                selected = subprocess.check_output([COMPILER, '-print-file-name=' + needed], text=True).strip()
                if not Path(selected).is_absolute() or not Path(selected).is_file():
                    raise RuntimeError('fixture compiler did not select required runtime file: ' + needed)
                selected = str(Path(selected).absolute())
                entry = file_entry('provider', selected, [needed, selected])
                images.append(entry)
                aliases.update(entry['aliases'])
        value = {'schema_version': 1, 'images': images, 'cwd': str(self.root),
                 'environment': {'LC_ALL': 'C', 'DISPLAY': ':fixture-declared-display',
                                 'LD_PRELOAD': str(self.evil), 'LD_LIBRARY_PATH': str(self.root)}}
        self.write_manifest(value)
        return value

    def write_manifest(self, value):
        self.manifest_path.write_text(json.dumps(value))

    def supervisor(self, mode='load', target=None, extras=(), timeout=10, auditor=None):
        arguments = [str(self.application), mode]
        if target is not None:
            arguments.append(str(target))
        arguments.extend(map(str, extras))
        return custody.Supervisor(self.manifest_path, self.record, timeout, arguments,
                                  audit_module=auditor or AUDIT_MODULE)

    def run_target(self, *args, **kwargs):
        with self.supervisor(*args, **kwargs) as supervisor:
            return supervisor.run()

    def assert_admitted(self, receipt):
        self.assertTrue(receipt['admitted'], receipt.get('failure'))
        self.assertTrue(receipt['activated'])
        self.assertTrue(receipt['preinit'])
        self.assertEqual(receipt['target_exit_code'], 0)
        self.assertTrue(receipt['target_reaped'])
        self.assertTrue(receipt['retained_through_reaping'])
        self.assertTrue(receipt['retained_fds_closed'])
        self.assertTrue(receipt['owned_tree_retired'])
        self.assertFalse(receipt['retirement_incomplete'])
        self.assertTrue(receipt['ownership']['wait_echild'])
        self.assertTrue(receipt['ownership']['root_status_accounted'])
        self.assertEqual(receipt['ownership']['remaining_trace_tids'], [])
        self.assertEqual(receipt['ownership']['rejected_tasks'], [])
        self.assertFalse(receipt['scope']['continuous_mapping_qualification'])
        self.assertFalse(receipt['scope']['cohort_qualified'])

    def events(self):
        return [json.loads(line) for line in Path(str(self.record) + '.events.jsonl').read_text().splitlines()]

    def test_late_soname_routes_sealed_bytes_after_original_path_substitution(self):
        self.manifest([self.late])
        with self.supervisor(target='libisle_late.so') as supervisor:
            supervisor.prepare()
            retained = [(image.fd, image.path, image.key) for image in supervisor.images]
            shutil.copyfile(self.evil, self.late)
            # Even replacement of the actual executable cannot change the staged app.
            shutil.copyfile(self.evil, self.application)
            receipt = supervisor.run()
        self.assert_admitted(receipt)
        self.assertIn('native-answer=42', receipt['streams']['stdout']['preview_utf8'])
        self.assertNotIn('UNADMITTED_CONSTRUCTOR', receipt['streams']['stdout']['preview_utf8'])
        self.assertNotIn('LD_PRELOAD', receipt['environment'])
        self.assertNotIn('LD_LIBRARY_PATH', receipt['environment'])
        for fd, path, key in retained:
            with self.assertRaises(OSError) as closed:
                os.fstat(fd)
            self.assertEqual(closed.exception.errno, errno.EBADF)
            self.assertFalse(Path(path).exists())
            image = next(image for image in receipt['images'] if image['supervisor_fd'] == fd)
            self.assertEqual((image['device_major'], image['device_minor'], image['inode']), key)
            self.assertEqual(image['seals'], 15)
        searches = [event for event in self.events() if event['kind'] == 'search']
        self.assertTrue(any(event['name'] == 'libisle_late.so' for event in searches))
        self.assertTrue(all(event['checkpoint']['config_readonly_witness'] for event in searches))
        identities = {image['index']: (image['device_major'], image['device_minor'], image['inode'])
                      for image in receipt['images']}
        for event in self.events():
            for mapped in event['checkpoint']['mapped_segments']:
                self.assertEqual((mapped['device_major'], mapped['device_minor'], mapped['inode']),
                                 identities[mapped['image_index']])
                self.assertLess(mapped['start'], mapped['end'])

    def test_approved_absolute_routes_sealed_bytes_after_original_path_substitution(self):
        self.manifest([self.late])
        with self.supervisor(target=self.late) as supervisor:
            supervisor.prepare()
            shutil.copyfile(self.evil, self.late)
            receipt = supervisor.run()
        self.assert_admitted(receipt)
        self.assertIn('native-answer=42', receipt['streams']['stdout']['preview_utf8'])
        self.assertNotIn('UNADMITTED_CONSTRUCTOR', receipt['streams']['stdout']['preview_utf8'])
        self.assertTrue(any(event['kind'] == 'search' and event['name'] == str(self.late)
                            for event in self.events()))

    def test_real_transitive_dependency_is_sealed_and_missing_transitive_is_refused(self):
        manifest = self.manifest([self.late, self.dependent])
        receipt = self.run_target(target='libisle_dependent.so', extras=['unused', 'dependent'])
        self.assert_admitted(receipt)
        self.assertIn('native-answer=43', receipt['streams']['stdout']['preview_utf8'])
        manifest['images'] = [image for image in manifest['images'] if image['path'] != str(self.late)]
        self.write_manifest(manifest)
        receipt = self.run_target(target='libisle_dependent.so', extras=['unused', 'dependent'])
        self.assertFalse(receipt['admitted'])
        self.assertFalse(receipt['activated'])
        self.assertTrue(receipt['retained_fds_closed'])

    def test_unadmitted_provider_is_refused_before_constructor_or_behavior(self):
        self.manifest()
        receipt = self.run_target(target=self.evil)
        self.assertFalse(receipt['admitted'])
        self.assertTrue(receipt['activated'])
        self.assertTrue(receipt['preinit'])
        self.assertNotIn('UNADMITTED_CONSTRUCTOR', receipt['streams']['stdout']['preview_utf8'])
        self.assertNotIn('native-answer=', receipt['streams']['stdout']['preview_utf8'])
        self.assertTrue(receipt['target_reaped'])

    def test_absolute_namespace_root_bypass_is_refused(self):
        self.manifest()
        receipt = self.run_target(mode='namespace', target=self.evil)
        self.assertFalse(receipt['admitted'])
        self.assertTrue(receipt['preinit'])
        self.assertNotIn('UNADMITTED_CONSTRUCTOR', receipt['streams']['stdout']['preview_utf8'])
        self.assertNotIn('native-answer=', receipt['streams']['stdout']['preview_utf8'])

    def test_sealed_namespace_root_has_distinct_object_namespace_and_bias_witness(self):
        self.manifest([self.late])
        with self.supervisor(mode='namespace', target=self.late) as supervisor:
            supervisor.prepare()
            sealed = next(image.path for image in supervisor.images if image.original == str(self.late))
            supervisor.arguments[2] = sealed
            receipt = supervisor.run()
        self.assert_admitted(receipt)
        self.assertIn('native-answer=42', receipt['streams']['stdout']['preview_utf8'])
        late_index = next(image['index'] for image in receipt['images'] if image['original_path'] == str(self.late))
        opens = [event for event in self.events() if event['kind'] == 'open' and event['image_index'] == late_index]
        self.assertTrue(opens)
        self.assertTrue(all(event['namespace_id'] not in (0, custody.NO_NAMESPACE) for event in opens))
        self.assertTrue(all(event['load_bias'] in event['checkpoint']['complete_load_biases'][str(late_index)]
                            for event in opens))

    def test_alias_alone_does_not_admit_original_inode_on_namespace_bypass(self):
        self.manifest([self.late])
        with self.supervisor(mode='namespace', target=self.late) as supervisor:
            supervisor.prepare()
            shutil.copyfile(self.evil, self.late)
            receipt = supervisor.run()
        # GNU dlmopen's absolute namespace root bypasses la_objsearch. An alias
        # is not evidence that the mapped original inode is our sealed inode.
        self.assertFalse(receipt['admitted'])
        self.assertNotIn('UNADMITTED_CONSTRUCTOR', receipt['streams']['stdout']['preview_utf8'])
        self.assertNotIn('native-answer=', receipt['streams']['stdout']['preview_utf8'])

    def test_nonexecuting_data_does_not_acquire_image_custody_or_block_real_provider(self):
        self.manifest([self.late])
        data = self.root / 'ordinary-data.bin'
        data.write_bytes(b'ordinary non-executable resource data\n' * 128)
        receipt = self.run_target(mode='map', target='libisle_late.so', extras=[data])
        self.assert_admitted(receipt)
        self.assertIn('native-answer=42', receipt['streams']['stdout']['preview_utf8'])
        rows = [mapping for event in self.events()
                for mapping in event['checkpoint']['unprotected_data_mappings']
                if mapping['inode'] == data.stat().st_ino]
        self.assertTrue(rows)
        self.assertTrue(all('x' not in row['permissions'] and not row['immutable_custody'] for row in rows))


    def test_transient_original_inode_exec_and_protection_are_refused_before_completion(self):
        self.manifest([self.late])
        for mode in ('transient-exec', 'transient-protect'):
            with self.subTest(mode=mode):
                receipt = self.run_target(mode=mode, target=self.late)
                self.assertFalse(receipt['admitted'])
                self.assertTrue(receipt['preinit'])
                self.assertTrue(receipt['target_reaped'])
                self.assertTrue(receipt['retained_fds_closed'])
                self.assertTrue(receipt['syscall_custody']['complete'])
                self.assertIsNotNone(receipt['syscall_custody']['failure'])
                self.assertNotIn('native-mapping-complete', receipt['streams']['stdout']['preview_utf8'])

    def test_retained_sealed_inode_supports_transient_exec_and_protection(self):
        self.manifest([self.late])
        for mode in ('transient-exec', 'transient-protect'):
            with self.subTest(mode=mode), self.supervisor(mode=mode, target=self.late) as supervisor:
                supervisor.prepare()
                image = next(image for image in supervisor.images if image.original == str(self.late))
                supervisor.arguments[2] = image.path
                receipt = supervisor.run()
                self.assert_admitted(receipt)
                self.assertIn('native-mapping-complete', receipt['streams']['stdout']['preview_utf8'])
                self.assertTrue(receipt['syscall_custody']['complete'])

    def test_concurrent_native_threads_map_protect_and_retire_under_quiescence(self):
        self.manifest([self.late])
        with self.supervisor(mode='threads', target=self.late) as supervisor:
            supervisor.prepare()
            image = next(image for image in supervisor.images if image.original == str(self.late))
            supervisor.arguments[2] = image.path
            receipt = supervisor.run()
        self.assert_admitted(receipt)
        self.assertIn('native-thread-mappings-complete', receipt['streams']['stdout']['preview_utf8'])
        self.assertTrue(receipt['syscall_custody']['complete'])
        self.assertEqual(receipt['syscall_custody']['remaining_tids'], [])
        metadata = [json.loads(line) for line in Path(receipt['syscall_journal']['path']).read_text().splitlines()]
        created = [event for event in metadata if event['kind'] == 'created']
        self.assertEqual(len(created), 2)
        self.assertTrue(all(event['tgid'] == receipt['pid'] for event in created))

    def test_worker_survives_leader_exit_with_actual_mapping_and_loader_admission(self):
        self.manifest([self.late])
        with self.supervisor(mode='leaderless', target=self.late) as supervisor:
            supervisor.prepare()
            image = next(image for image in supervisor.images if image.original == str(self.late))
            supervisor.arguments[2] = image.path
            receipt = supervisor.run()
        self.assert_admitted(receipt)
        self.assertIn('native-leaderless-work-complete', receipt['streams']['stdout']['preview_utf8'])
        self.assertTrue(receipt['syscall_custody']['complete'])

    def test_eventless_escaped_clone3_child_is_retired_before_image_release(self):
        self.manifest()
        receipt = self.run_target(mode='escaped-clone3', timeout=1)
        if receipt['target_exit_code'] == 77 and receipt['admitted']:
            self.skipTest('native kernel did not implement this clone3 creation')
        self.assertFalse(receipt['admitted'])
        self.assertTrue(receipt['target_reaped'])
        self.assertTrue(receipt['retained_fds_closed'])
        self.assertTrue(receipt['owned_tree_retired'])
        self.assertTrue(receipt['ownership']['wait_echild'])
        unexpected = {row['tid'] for row in receipt['ownership']['rejected_tasks']}
        self.assertTrue(unexpected - {receipt['pid']})
        reaped = {row['tid'] for row in receipt['ownership']['reaped_tasks']}
        self.assertTrue(unexpected <= reaped)
        for line in receipt['streams']['stdout']['preview_utf8'].splitlines():
            if line.startswith('native-escaped-child='):
                self.assertFalse(Path('/proc/' + line.partition('=')[2]).exists())

    def test_legacy_untraced_creation_is_refused_before_creating_a_child(self):
        self.manifest()
        receipt = self.run_target(mode='escaped-clone')
        self.assertFalse(receipt['admitted'])
        self.assertTrue(receipt['preinit'])
        self.assertTrue(receipt['owned_tree_retired'])
        self.assertTrue(receipt['retained_fds_closed'])
        self.assertNotIn('native-escaped-child=', receipt['streams']['stdout']['preview_utf8'])
        self.assertEqual(receipt['ownership']['observed_direct_children'], [receipt['pid']])

    def test_pre_activation_session_escape_and_doublefork_are_fully_retired(self):
        # A real DSO without la_version lets glibc continue without activation.
        # Ownership must still retire its actual escaped descendants.
        self.manifest(auditor=self.late)
        for mode in ('escaped-clone', 'escaped-doublefork'):
            with self.subTest(mode=mode):
                receipt = self.run_target(mode=mode, timeout=1, auditor=self.late)
                self.assertFalse(receipt['admitted'])
                self.assertFalse(receipt['activated'])
                self.assertTrue(receipt['target_reaped'])
                self.assertTrue(receipt['owned_tree_retired'])
                self.assertTrue(receipt['retained_fds_closed'])
                self.assertTrue(receipt['ownership']['wait_echild'])
                unexpected = {row['tid'] for row in receipt['ownership']['rejected_tasks']}
                self.assertTrue(unexpected - {receipt['pid']})
                reaped = {row['tid'] for row in receipt['ownership']['reaped_tasks']}
                self.assertTrue(unexpected <= reaped)
                for line in receipt['streams']['stdout']['preview_utf8'].splitlines():
                    if line.startswith('native-escaped-child='):
                        self.assertFalse(Path('/proc/' + line.partition('=')[2]).exists())

    def test_public_launch_does_not_consume_or_signal_unrelated_caller_child(self):
        self.manifest()
        unrelated = subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(30)'])
        try:
            receipt = custody.launch(self.manifest_path, self.record, 10, [str(self.application), 'zero'],
                                     audit_module=AUDIT_MODULE)
            self.assert_admitted(receipt)
            self.assertIsNone(unrelated.poll())
        finally:
            unrelated.terminate()
            self.assertEqual(unrelated.wait(timeout=5), -signal.SIGTERM)

    def test_new_process_and_exec_are_refused_without_unobserved_native_behavior(self):
        self.manifest()
        for mode, marker in (('fork', 'native-child-ran'), ('exec', 'native-zero')):
            with self.subTest(mode=mode):
                receipt = self.run_target(mode=mode)
                self.assertFalse(receipt['admitted'])
                self.assertTrue(receipt['preinit'])
                self.assertTrue(receipt['target_reaped'])
                self.assertTrue(receipt['retained_fds_closed'])
                self.assertTrue(receipt['syscall_custody']['complete'])
                self.assertNotIn(marker, receipt['streams']['stdout']['preview_utf8'])

    def test_implicit_execution_personality_and_non_native_syscalls_are_refused(self):
        self.manifest()
        for mode in ('read-implies-exec', 'x32'):
            with self.subTest(mode=mode):
                receipt = self.run_target(mode=mode)
                self.assertFalse(receipt['admitted'])
                self.assertTrue(receipt['preinit'])
                self.assertTrue(receipt['target_reaped'])
                self.assertTrue(receipt['retained_fds_closed'])
                self.assertTrue(receipt['syscall_custody']['complete'])
                self.assertNotIn('native-abi-attempt-returned', receipt['streams']['stdout']['preview_utf8'])

    def test_source_control_channel_failure_is_fail_closed(self):
        self.manifest([self.late])
        receipt = self.run_target(mode='channel', target='libisle_late.so')
        self.assertFalse(receipt['admitted'])
        self.assertTrue(receipt['preinit'])
        self.assertEqual(receipt['target_exit_code'], 126)
        self.assertNotIn('native-answer=', receipt['streams']['stdout']['preview_utf8'])

    def test_bad_configuration_handoff_cannot_silently_run_application(self):
        self.manifest([self.late])
        with self.supervisor(target='libisle_late.so') as supervisor:
            supervisor.prepare()
            os.close(supervisor.config_fd)
            supervisor.owned_fds.remove(supervisor.config_fd)
            supervisor.config_fd = supervisor.own(custody.high_fd(os.open(os.devnull, os.O_RDONLY | os.O_CLOEXEC)))
            receipt = supervisor.run()
        self.assertFalse(receipt['admitted'])
        self.assertFalse(receipt['activated'])
        self.assertNotIn('native-answer=', receipt['streams']['stdout']['preview_utf8'])
        self.assertTrue(receipt['target_reaped'])

    def test_wrong_serial_ack_terminates_real_native_auditor(self):
        self.manifest([self.late])
        with self.supervisor(target='libisle_late.so') as supervisor:
            supervisor.prepare()
            supervisor.spawn()
            supervisor.parent_socket.settimeout(5)
            packet = supervisor.parent_socket.recv(custody.EVENT.size + 4096)
            event = custody.EVENT.unpack_from(packet)
            self.assertEqual(event[2], 1)
            supervisor.parent_socket.send(custody.ACK.pack(custody.ACK_MAGIC, custody.VERSION, event[3] + 1, 1))
            deadline = time.monotonic() + 5
            while not supervisor.reaped and time.monotonic() < deadline:
                supervisor.reap()
                if not supervisor.reaped:
                    time.sleep(0.01)
            self.assertTrue(supervisor.reaped, 'native callback did not reject mismatched synchronous ACK')
            self.assertEqual(os.waitstatus_to_exitcode(supervisor.wait_status), 126)

    def test_zero_exit_without_activation_is_not_admission(self):
        # A real DSO lacking la_version makes glibc ignore --audit and continue.
        # That loader behavior is precisely why exit(0) is not admission evidence.
        self.manifest(auditor=self.late)
        receipt = self.run_target(mode='zero', auditor=str(self.late))
        self.assertFalse(receipt['admitted'])
        self.assertFalse(receipt['activated'])
        self.assertTrue(receipt['target_reaped'])

    def test_timeout_signal_and_nonzero_exit_have_explicit_receipts(self):
        self.manifest()
        receipt = self.run_target(mode='sleep', timeout=1)
        self.assertFalse(receipt['admitted'])
        self.assertTrue(receipt['target_reaped'])
        self.assertTrue(receipt['retained_fds_closed'])
        receipt = self.run_target(mode='signal')
        self.assertFalse(receipt['admitted'])
        self.assertEqual(receipt['target_exit_code'], -signal.SIGTERM)
        receipt = self.run_target(mode='exit7')
        self.assertTrue(receipt['admitted'], receipt['failure'])
        self.assertEqual(receipt['target_exit_code'], 7)
        self.assertEqual(receipt['status'], 'completed')

    def test_changed_whole_file_hash_and_ambiguous_alias_are_refused(self):
        manifest = self.manifest([self.late])
        next(image for image in manifest['images'] if image['path'] == str(self.late))['expected_sha256'] = '0' * 64
        self.write_manifest(manifest)
        receipt = self.run_target(target='libisle_late.so')
        self.assertFalse(receipt['admitted'])
        self.assertTrue(receipt['retained_fds_closed'])
        manifest = self.manifest([self.late])
        manifest['images'][0]['aliases'].append('libisle_late.so')
        self.write_manifest(manifest)
        receipt = self.run_target(target='libisle_late.so')
        self.assertFalse(receipt['admitted'])
        self.assertTrue(receipt['retained_fds_closed'])

    def test_cli_runs_actual_target_and_preserves_parent_reserved_descriptors(self):
        self.manifest([self.late])
        command = [sys.executable, str(Path(custody.__file__).absolute()), 'launch',
                   '--manifest', str(self.manifest_path), '--record', str(self.record),
                   '--timeout', '10', '--audit-module', AUDIT_MODULE, '--',
                   str(self.application), 'load', 'libisle_late.so']
        process = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=15)
        self.assertEqual(process.returncode, 0, process.stderr.decode(errors='replace'))
        self.assert_admitted(json.loads(self.record.read_text()))
        with self.supervisor(mode='zero') as supervisor:
            supervisor.prepare()
            backups = {}
            temporary = []
            try:
                for target in (custody.CONFIG_FD, custody.CONTROL_FD):
                    try:
                        backups[target] = fcntl.fcntl(target, fcntl.F_DUPFD_CLOEXEC, 500)
                    except OSError as error:
                        if error.errno != errno.EBADF:
                            raise
                        backups[target] = None
                    fd = os.open(os.devnull, os.O_RDONLY | os.O_CLOEXEC)
                    temporary.append(fd)
                    os.dup2(fd, target, inheritable=False)
                before = {target: os.fstat(target) for target in backups}
                receipt = supervisor.run()
                for target in backups:
                    after = os.fstat(target)
                    self.assertEqual((after.st_dev, after.st_ino), (before[target].st_dev, before[target].st_ino))
            finally:
                for target, backup in backups.items():
                    if backup is None:
                        os.close(target)
                    else:
                        os.dup2(backup, target, inheritable=False)
                        os.close(backup)
                for fd in temporary:
                    os.close(fd)
        self.assert_admitted(receipt)


def main():
    global AUDIT_MODULE, COMPILER
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--audit-module', required=True)
    parser.add_argument('--compiler', default=shutil.which('cc'))
    options, arguments = parser.parse_known_args()
    if not options.compiler:
        parser.error('a real native C compiler is required')
    AUDIT_MODULE = str(Path(options.audit_module).absolute())
    COMPILER = options.compiler
    # CTest invokes this as a dedicated, single-threaded native fixture owner.
    from runtime_custody_owner import declare_isolated_owner
    declare_isolated_owner()
    unittest.main(argv=[sys.argv[0], *arguments])


if __name__ == '__main__':
    main()
