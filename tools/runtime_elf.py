"""Read-only ELF identities shared by private runtime custody tools."""
import hashlib
import os
from pathlib import Path
import re
import subprocess


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def fail(message):
    raise ValueError(message)


def elf_dynamic(path):
    """Inspect ELF metadata, never load or execute an application/DSO."""
    with Path(path).open('rb') as stream:
        magic = stream.read(4)
    if magic != b'\x7fELF':
        return None
    output = subprocess.check_output(['readelf', '-d', str(path)], text=True,
                                     env=dict(os.environ, LC_ALL='C'))
    soname = re.findall(r'\(SONAME\).*\[([^\]]+)\]', output)
    needed = re.findall(r'\(NEEDED\).*\[([^\]]+)\]', output)
    if any('/' in name or '\\' in name for name in needed):
        fail('non-system DSO has a non-relocatable DT_NEEDED path: ' + Path(path).name)
    return {'soname': soname[0] if soname else None, 'needed': needed,
            'runpath': re.findall(r'\(RUNPATH\).*\[([^\]]*)\]', output),
            'rpath': re.findall(r'\(RPATH\).*\[([^\]]*)\]', output)}


def elf_identity(path):
    """Read native ELF identity/program headers; callers supply admission policy."""
    dynamic = elf_dynamic(path)
    if dynamic is None:
        fail('not an ELF image: ' + str(path))
    output = subprocess.check_output(
        ['readelf', '--wide', '--file-header', '--program-headers', '--dynamic', str(path)],
        text=True, env=dict(os.environ, LC_ALL='C'), stderr=subprocess.PIPE)

    def field(name):
        match = re.search(r'^\s*' + re.escape(name) + r':\s*(.+)$', output, re.MULTILINE)
        if not match:
            fail('missing ELF header field ' + name)
        return match.group(1).strip()

    segments = []
    for match in re.finditer(
            r'^\s*(\S+)\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+'
            r'(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)'
            r'\s+([RWE ]*?)\s+(0x[0-9a-fA-F]+)\s*$', output, re.MULTILINE):
        kind, offset, address, physical, filesz, memsz, flags, alignment = match.groups()
        segments.append({'type': kind, 'offset': int(offset, 16),
                         'vaddr': int(address, 16), 'paddr': int(physical, 16),
                         'filesz': int(filesz, 16), 'memsz': int(memsz, 16),
                         'flags': flags.replace(' ', ''), 'align': int(alignment, 16)})
    if not segments:
        fail('ELF has no readable program headers: ' + str(path))
    interpreters = re.findall(r'\[Requesting program interpreter:\s*([^\]]+)\]', output)
    tags = re.findall(r'^\s*0x[0-9a-fA-F]+\s+\(([^)]+)\)\s*(.*)$', output, re.MULTILINE)
    return {'class': field('Class'), 'data': field('Data'),
            'machine': field('Machine'), 'type': field('Type').split()[0],
            'entry': int(field('Entry point address'), 16),
            'segments': segments, 'interpreters': interpreters,
            'dynamic_tags': [{'tag': name, 'value': value} for name, value in tags],
            'dynamic': dynamic}
