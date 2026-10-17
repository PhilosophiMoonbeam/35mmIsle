#!/usr/bin/env python3
"""Materialize only reviewed game definitions; never read a mutable source file."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile


def git(repository, *arguments):
    environment = {key: value for key, value in os.environ.items() if not key.startswith("GIT_")}
    environment["GIT_CONFIG_NOSYSTEM"] = "1"
    environment["GIT_CONFIG_GLOBAL"] = os.devnull
    completed = subprocess.run(
        ["git", "-C", str(repository), *arguments],
        check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=environment,
    )
    return completed.stdout


def publish(path, content):
    data = content.encode("utf-8")
    if path.is_file() and path.read_bytes() == data:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(data)
        os.replace(temporary, path)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def prepare(repository, manifest_path, output_path, evidence_path):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    revision = manifest["revision"]
    if manifest["schema_version"] != 1 or re.fullmatch(r"[0-9a-f]{40}", revision) is None:
        raise ValueError("unsupported engine manifest/revision")
    if git(repository, "cat-file", "-t", revision).strip() != b"commit":
        raise ValueError("engine authority is not a commit")
    sources = {}
    for path, identity in manifest["source_files"].items():
        # This adapter has no SDK/OS/assembly input path or include-tree import.
        if path not in ("src/sys/om.c", "src/AA0650.c", "src/sys/matrix.c", "src/sys/render.c",
                        "include/sys/mtx.h", "src/sys/vector.c", "include/sys/vector.h"):
            raise ValueError("unadmitted source path: " + path)
        data = git(repository, "show", revision + ":" + path)
        blob = git(repository, "rev-parse", revision + ":" + path).decode("ascii").strip()
        if blob != identity["git_blob"] or hashlib.sha256(data).hexdigest() != identity["sha256"]:
            raise ValueError("engine source identity mismatch: " + path)
        sources[path] = data.decode("utf-8").splitlines(keepends=True)
    # Hash the original native seams that implement defined MIPS arithmetic and
    # ownership/photo concatenation. These are local sources, never SDK input.
    native_support = {}
    root = Path(__file__).resolve().parent.parent
    for path, identity in manifest["native_support"].items():
        if path not in ("src/engine.c", "src/engine_math.h", "src/contextual_model_math.h",
                        "src/engine_internal.h", "src/source_factory_resources.h",
                        "include/35mmisle/engine.h"):
            raise ValueError("unadmitted native support path: " + path)
        digest = hashlib.sha256((root / path).read_bytes()).hexdigest()
        if digest != identity["sha256"]:
            raise ValueError("native support identity mismatch: " + path)
        native_support[path] = identity
    output = ['/* Build-private selected game definitions; see source evidence. */\n',
              '#include "engine_internal.h"\n\n']
    identities = []
    symbols = set()
    for unit in manifest["definitions"]:
        symbol = unit["symbol"]
        if re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", symbol) is None or symbol in symbols:
            raise ValueError("invalid/duplicate engine symbol")
        symbols.add(symbol)
        lines = sources[unit["path"]]
        first, last = unit["first"], unit["last"]
        if not isinstance(first, int) or not isinstance(last, int) or not 1 <= first <= last <= len(lines):
            raise ValueError("engine source span outside file: " + symbol)
        selected = "".join(lines[first - 1:last])
        if hashlib.sha256(selected.encode()).hexdigest() != unit["source_sha256"]:
            raise ValueError("engine definition identity mismatch: " + symbol)
        native = selected
        for change in unit["changes"]:
            if not change["find"] or native.count(change["find"]) != 1:
                raise ValueError("native change does not match exactly once: " + symbol)
            native = native.replace(change["find"], change["replace"], 1)
        digest = hashlib.sha256(native.encode()).hexdigest()
        if digest != unit["native_sha256"]:
            raise ValueError("native definition identity mismatch: " + symbol)
        output.append('#line %d "snap@%s/%s"\n' % (first, revision, unit["path"]))
        output.append(native)
        output.append("\n")
        identities.append({key: unit[key] for key in
                           ("symbol", "path", "first", "last", "source_sha256", "native_sha256")})
    if not identities:
        raise ValueError("empty engine allowlist")
    source = "".join(output)
    evidence = {
        "revision": revision,
        "manifest_sha256": hashlib.sha256(manifest_bytes).hexdigest(),
        "generator_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "generated_source_sha256": hashlib.sha256(source.encode()).hexdigest(),
        "source_files": manifest["source_files"],
        "definitions": identities,
        "native_support": native_support,
        # Historical static observations are evidence, not ROM/runtime inputs
        # to preparation. The private ROM is neither copied nor published here.
        "rom_observations": manifest["rom_observations"],
        "scope": "Private game C compilation; no SDK implementation or public asset input",
    }
    # Validate everything before replacing the prior valid generated source.
    publish(output_path, source)
    publish(evidence_path, json.dumps(evidence, indent=2) + "\n")
    print("Prepared %d reviewed native game definitions at %s" % (len(identities), revision))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    arguments = parser.parse_args()
    try:
        prepare(arguments.repository, arguments.manifest, arguments.output, arguments.evidence)
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        parser.exit(1, "Private native engine preparation failed: %s\n" % error)


if __name__ == "__main__":
    main()
