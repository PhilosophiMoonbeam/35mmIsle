#!/usr/bin/env python3
"""Synthetic archive/privacy regressions; no ROM or native executable is used."""
import hashlib
import json
import io
from pathlib import Path
import sys
import tarfile
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from prepare_runtime_candidate import inspect_archive, public_name, hash_stream
from runtime_composition import CompositionError
from runtime_composition import digest, llvm_identity


class RuntimeCandidateTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.data = b'independently authored synthetic product bytes'
        self.expected = {'bin/35mmisle': {'kind': 'file', 'bytes': len(self.data),
                                        'sha256': hashlib.sha256(self.data).hexdigest()}}

    def archive(self, entries):
        archive = self.root / 'candidate.tar.gz'
        with tarfile.open(archive, 'w:gz') as output:
            for name, content, target in entries:
                member = tarfile.TarInfo(name)
                if name.endswith('/'):
                    member.type = tarfile.DIRTYPE
                    output.addfile(member)
                    continue
                if target is not None:
                    member.type = tarfile.SYMTYPE
                    member.linkname = target
                    output.addfile(member)
                else:
                    member.size = len(content)
                    output.addfile(member, io.BytesIO(content))
        return archive

    def test_exact_tgz(self):
        archive = self.archive([('product/', b'', None), ('product/bin/', b'', None),
                                ('product/bin/35mmisle', self.data, None)])
        inspect_archive(archive, 'product', self.expected)

    def test_exact_zip(self):
        archive = self.root / 'candidate.zip'
        with zipfile.ZipFile(archive, 'w') as output:
            output.writestr('product/', b'')
            output.writestr('product/bin/', b'')
            output.writestr('product/bin/35mmisle', self.data)
        inspect_archive(archive, 'product', self.expected)

    def test_extra_missing_duplicate_changed_and_escaping_members_reject(self):
        valid = ('product/bin/35mmisle', self.data, None)
        cases = [[valid, ('product/bin/extra', b'x', None)], [], [valid, valid],
                 [('product/bin/35mmisle', b'changed', None)],
                 [('../product/bin/35mmisle', self.data, None)],
                 [('other/bin/35mmisle', self.data, None)]]
        for entries in cases:
            with self.subTest(entries=entries), self.assertRaises(CompositionError):
                inspect_archive(self.archive(entries), 'product', self.expected)

    def test_unexpected_empty_directories_reject_in_both_formats(self):
        entries = [('product/', b'', None), ('product/bin/', b'', None),
                   ('product/bin/35mmisle', self.data, None),
                   ('product/empty/', b'', None)]
        with self.assertRaises(CompositionError):
            inspect_archive(self.archive(entries), 'product', self.expected)
        archive = self.root / 'empty-directory.zip'
        with zipfile.ZipFile(archive, 'w') as output:
            for name, content, _ in entries:
                output.writestr(name, content)
        with self.assertRaises(CompositionError):
            inspect_archive(archive, 'product', self.expected)

    def test_sdk_notice_requires_exact_producer_selection_in_both_formats(self):
        name = 'share/licenses/35mmisle/dependencies/sdk/SDL3/LICENSE.txt'
        content = b'Independently authored synthetic dependency license notice.'
        record = {'path': name, 'sha256': hashlib.sha256(content).hexdigest()}
        expected = dict(self.expected)
        expected[name] = {'kind': 'file', 'bytes': len(content), 'sha256': record['sha256']}
        directories = ['product/', 'product/bin/', 'product/share/',
                       'product/share/licenses/', 'product/share/licenses/35mmisle/',
                       'product/share/licenses/35mmisle/dependencies/',
                       'product/share/licenses/35mmisle/dependencies/sdk/',
                       'product/share/licenses/35mmisle/dependencies/sdk/SDL3/']
        entries = [(directory, b'', None) for directory in directories]
        entries += [('product/bin/35mmisle', self.data, None), ('product/' + name, content, None)]
        archives = [self.archive(entries), self.root / 'sdk-notice.zip']
        with zipfile.ZipFile(archives[1], 'w') as output:
            for entry, data, _ in entries:
                output.writestr(entry, data)
        for archive in archives:
            with self.subTest(archive=archive.name):
                inspect_archive(archive, 'product', expected, [record])
                with self.assertRaises(CompositionError):
                    inspect_archive(archive, 'product', expected)
                with self.assertRaises(CompositionError):
                    inspect_archive(archive, 'product', expected,
                                    [dict(record, sha256='0' * 64)])
                nonregular = dict(expected)
                nonregular[name] = dict(expected[name], kind='symlink', target='elsewhere')
                with self.assertRaises(CompositionError):
                    inspect_archive(archive, 'product', nonregular, [record])
                unselected = dict(expected)
                unselected[name.replace('LICENSE.txt', 'UNSELECTED.txt')] = expected[name]
                with self.assertRaises(CompositionError):
                    inspect_archive(archive, 'product', unselected, [record])

    def test_sdk_selection_does_not_admit_headers_libraries_or_sdk_elsewhere(self):
        for name in ('share/licenses/35mmisle/dependencies/sdk/include/sdk.h',
                     'share/licenses/35mmisle/dependencies/sdk/include/LICENSE.hpp',
                     'share/licenses/35mmisle/dependencies/sdk/lib/provider.lib',
                     'share/licenses/35mmisle/dependencies/sdk/lib/provider.so.1',
                     'share/licenses/35mmisle/dependencies/sdk/settings.json',
                     'share/35mmisle/sdk/LICENSE.txt'):
            content = b'synthetic excluded SDK material'
            record = {'path': name, 'sha256': hashlib.sha256(content).hexdigest()}
            expected = dict(self.expected)
            expected[name] = {'kind': 'file', 'bytes': len(content), 'sha256': record['sha256']}
            archive = self.archive([('product/bin/35mmisle', self.data, None),
                                    ('product/' + name, content, None)])
            with self.subTest(name=name), self.assertRaises(CompositionError):
                inspect_archive(archive, 'product', expected, [record])

    def test_hash_bound_sdk_notice_still_rejects_secret_bytes(self):
        name = 'share/licenses/35mmisle/dependencies/sdk/LICENSE.txt'
        content = b'-----BEGIN ' + b'PRIVATE KEY-----'
        record = {'path': name, 'sha256': hashlib.sha256(content).hexdigest()}
        expected = dict(self.expected)
        expected[name] = {'kind': 'file', 'bytes': len(content), 'sha256': record['sha256']}
        archive = self.archive([('product/bin/35mmisle', self.data, None),
                                ('product/' + name, content, None)])
        with self.assertRaises(CompositionError):
            inspect_archive(archive, 'product', expected, [record])

    def test_exact_tgz_symlink(self):
        expected = dict(self.expected)
        expected['lib/35mmisle/libsynthetic.so'] = {'kind': 'symlink', 'target': '../../bin/35mmisle'}
        archive = self.archive([('product/bin/35mmisle', self.data, None),
                                ('product/lib/35mmisle/libsynthetic.so', b'', '../../bin/35mmisle')])
        inspect_archive(archive, 'product', expected)
        expected['lib/35mmisle/libsynthetic.so']['target'] = '/outside'
        with self.assertRaises(CompositionError):
            inspect_archive(archive, 'product', expected)

    def test_hardlinks_reject(self):
        archive = self.root / 'hardlink.tar.gz'
        with tarfile.open(archive, 'w:gz') as output:
            member = tarfile.TarInfo('product/bin/35mmisle')
            member.type = tarfile.LNKTYPE
            member.linkname = 'outside'
            output.addfile(member)
        with self.assertRaises(CompositionError):
            inspect_archive(archive, 'product', self.expected)

    def test_private_and_obsolete_paths_reject(self):
        for name in ('bin/game.z64', 'share/35mmisle/profiles/save.json',
                     'share/35mmisle/captures/frame.png', 'build/CMakeCache.txt',
                     'share/licenses/35mmisle/dependencies/lightning/COPYING',
                     'bin/35mmisle-rsp-probe', 'bin/private.lib', '../escape'):
            with self.subTest(name=name), self.assertRaises(CompositionError):
                public_name(name)
        public_name('share/licenses/35mmisle/dependencies/llvm/llvm/LICENSE.TXT')
        public_name('src/original.cpp', runtime=False)

    def test_secret_bytes_reject_even_across_blocks(self):
        prefix = b'x' * (1024 * 1024 - 10)
        with self.assertRaises(CompositionError):
            hash_stream(io.BytesIO(prefix + b'-----BEGIN ' + b'PRIVATE KEY-----'))


class LLVMIdentityTests(unittest.TestCase):
    def test_exact_source_receipt_and_stale_cache_rejection(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            project = root / 'project'
            source = root / 'llvm-source'
            build = root / 'llvm-build'
            for directory in (project / 'cmake', project / 'tools', source / 'llvm', build):
                directory.mkdir(parents=True)
            pin = {'version': '21.1.8',
                   'commit': '2078da43e25a4623cab2d0d60decddf709aaea28',
                   'archive_sha256': '4633a23617fa31a3ea51242586ea7fb1da7140e426bd62fc164261fe036aa142',
                   'source_inventory_sha256': 'a' * 64}
            (project / 'cmake/llvm-dependencies.json').write_text(
                json.dumps({'dependencies': [pin]}), encoding='utf-8')
            (project / 'tools/prepare_llvm.py').write_text('# synthetic recipe\\n', encoding='utf-8')
            (source / 'llvm/LICENSE.TXT').write_text('synthetic notice\\n', encoding='utf-8')
            cache = build / 'CMakeCache.txt'
            cache.write_text('synthetic selected build cache\\n', encoding='utf-8')
            archive = build / 'synthetic.a'
            archive.write_bytes(b'synthetic compiled library')
            artifacts = root / 'build-artifacts.json'
            artifacts.write_text(json.dumps({
                'source_inventory_sha256': pin['source_inventory_sha256'],
                'cache_sha256': digest(cache),
                'files': {'synthetic.a': digest(archive)}}), encoding='utf-8')
            facts = {'cache_sha256': digest(cache), 'compiler_id': 'GNU',
                     'compiler_version': 'synthetic', 'compiler_sha256': 'b' * 64,
                     'system': 'Linux', 'processor': 'x86_64', 'pointer_size': '8',
                     'build_type': 'Release', 'targets': 'X86', 'shared': False,
                     'msvc_runtime': '', 'standard_library': 'libstdc++',
                     'source_inventory_sha256': pin['source_inventory_sha256']}
            facts['artifact_inventory_sha256'] = digest(artifacts)
            receipt = dict(pin, license_files=['llvm/LICENSE.TXT'], build=facts)
            receipt_path = root / 'provenance.json'
            receipt_path.write_text(json.dumps(receipt), encoding='utf-8')
            inputs = {'llvm_root': str(source), 'llvm_build': str(build),
                      'source_root': str(project), 'llvm_version': '21.1.8',
                      'llvm_provenance': str(receipt_path)}
            identity = llvm_identity(inputs)
            self.assertEqual(identity['build']['standard_library'], 'libstdc++')
            self.assertNotIn(str(root), json.dumps(identity))
            archive.write_bytes(b'changed compiled library')
            with self.assertRaises(CompositionError):
                llvm_identity(inputs)
            archive.write_bytes(b'synthetic compiled library')
            cache.write_text('stale changed cache\\n', encoding='utf-8')
            with self.assertRaises(CompositionError):
                llvm_identity(inputs)


if __name__ == '__main__':
    unittest.main()
