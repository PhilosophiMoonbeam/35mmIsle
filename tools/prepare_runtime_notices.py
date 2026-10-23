#!/usr/bin/env python3
"""Prepare only pinned license excerpts; never export source bodies or font files."""

import argparse
import ast
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import sys
import zlib


def digest(data):
    return hashlib.sha256(data).hexdigest()


def scoped_path(root, relative):
    path = PurePosixPath(relative)
    if path.is_absolute() or not path.parts or any(p in (".", "..") for p in path.parts):
        raise ValueError("invalid scoped notice path: " + relative)
    result = root.joinpath(*path.parts)
    if not result.resolve().is_relative_to(root.resolve()):
        raise ValueError("notice path escapes its selected root: " + relative)
    return result


def decompress_stb(data, expected_size):
    """Decode the bounded stb stream format used by the two selected ImGui fonts."""
    if data[:8] != bytes.fromhex("57bc000000000000"):
        raise ValueError("unexpected embedded font compression header")
    if int.from_bytes(data[8:12], "big") != expected_size:
        raise ValueError("unexpected embedded font length")
    output = bytearray()
    offset = 16

    def number(start, length):
        block = data[start:start + length]
        if len(block) != length:
            raise ValueError("truncated embedded font stream")
        return int.from_bytes(block, "big")

    while offset < len(data):
        token = data[offset]
        literal = False
        distance = 0
        if token >= 0x80:
            distance, length, step = number(offset + 1, 1) + 1, token - 0x80 + 1, 2
        elif token >= 0x40:
            distance, length, step = number(offset, 2) - 0x4000 + 1, number(offset + 2, 1) + 1, 3
        elif token >= 0x20:
            literal, length, step = True, token - 0x20 + 1, 1
        elif token >= 0x18:
            distance, length, step = number(offset, 3) - 0x180000 + 1, number(offset + 3, 1) + 1, 4
        elif token >= 0x10:
            distance, length, step = number(offset, 3) - 0x100000 + 1, number(offset + 3, 2) + 1, 5
        elif token >= 0x08:
            literal, length, step = True, number(offset, 2) - 0x0800 + 1, 2
        elif token == 0x07:
            literal, length, step = True, number(offset + 1, 2) + 1, 3
        elif token == 0x06:
            distance, length, step = number(offset + 1, 3) + 1, number(offset + 4, 1) + 1, 5
        elif token == 0x04:
            distance, length, step = number(offset + 1, 3) + 1, number(offset + 4, 2) + 1, 6
        elif data[offset:offset + 2] == b"\x05\xfa":
            if len(output) != expected_size or zlib.adler32(output) != number(offset + 2, 4):
                raise ValueError("embedded font checksum mismatch")
            return bytes(output)
        else:
            raise ValueError("unexpected embedded font token")
        if len(output) + length > expected_size:
            raise ValueError("embedded font output exceeds pinned length")
        if literal:
            block = data[offset + step:offset + step + length]
            if len(block) != length:
                raise ValueError("truncated embedded font literal")
            output.extend(block)
            offset += step + length
        else:
            if not 0 < distance <= len(output):
                raise ValueError("invalid embedded font match distance")
            for _ in range(length):
                output.append(output[-distance])
            offset += step
    raise ValueError("unterminated embedded font stream")


def font_bytes(source, font):
    if font["encoding"] == "base85-stb":
        match = re.search(rb"fontawesome_compressed_data_base85\[[^]]+\]\s*=\s*(.*?);\s*$", source, re.S)
        if match is None:
            raise ValueError("missing pinned FontAwesome array")
        # C permits \? to avoid trigraphs; Python string literals do not decode it.
        encoded = "".join(ast.literal_eval(s.replace(b"\\?", b"?").decode("ascii"))
                          for s in re.findall(rb'"(?:[^"\\]|\\.)*"', match.group(1)))
        if len(encoded) != 155710 or len(encoded) % 5:
            raise ValueError("unexpected FontAwesome base85 length")
        compressed = bytearray()
        for offset in range(0, len(encoded), 5):
            digits = [ord(c) - (36 if ord(c) >= 92 else 35) for c in encoded[offset:offset + 5]]
            if any(not 0 <= d < 85 for d in digits):
                raise ValueError("unexpected FontAwesome base85 digit")
            compressed.extend(sum(d * 85 ** i for i, d in enumerate(digits)).to_bytes(4, "little"))
    elif font["encoding"] == "u8-stb":
        match = re.search(rb"proggy_clean_ttf_compressed_data\[[^]]+\]\s*=\s*\{([^}]+)\}", source, re.S)
        if match is None:
            raise ValueError("missing pinned ProggyClean array")
        compressed = bytes(map(int, re.findall(rb"\d+", match.group(1))))
    else:
        raise ValueError("unknown embedded font encoding")
    return decompress_stb(compressed, font["size"])


def prepare(args):
    manifest_path = Path(args.manifest)
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    if manifest["schema"] != 1:
        raise ValueError("unsupported runtime notice manifest schema")
    roots = {name: Path(getattr(args, name)) for name in ("lus", "imgui", "monocypher", "stb", "json")}
    roots["compliance"] = manifest_path.parent
    if args.json_version not in manifest["json"]:
        raise ValueError("unaudited selected nlohmann_json version: " + args.json_version)
    selected_json = manifest["json"][args.json_version]
    layout = "modular" if scoped_path(roots["json"], "nlohmann/detail").is_dir() else "single"
    json_inputs = selected_json[layout]
    json_variant = {"kind": "upstream", "commit": selected_json["commit"]}
    matches = lambda entries: all(
        digest(scoped_path(roots["json"], e["path"]).read_bytes()) == e["sha256"]
        for e in entries)
    if not matches(json_inputs):
        choices = []
        canonical_paths = {e["path"] for e in json_inputs}
        for specification in selected_json.get("distributions", []):
            raw = scoped_path(roots["compliance"], specification["path"]).read_bytes()
            if digest(raw) != specification["sha256"]:
                raise ValueError("pinned JSON distribution notice input changed")
            variant = json.loads(raw)
            if variant["schema"] != 1 or variant["json_version"] != args.json_version:
                raise ValueError("invalid JSON distribution notice identity")
            if variant["layout"] != layout:
                continue
            overrides = variant["sha256_overrides"]
            if not overrides or not set(overrides) <= canonical_paths:
                raise ValueError("JSON distribution overrides unknown header paths")
            candidate = [dict(e, sha256=overrides.get(e["path"], e["sha256"]))
                         for e in json_inputs]
            if matches(candidate):
                choices.append((candidate, {"kind": "distribution",
                    "manifest_sha256": specification["sha256"],
                    "name": variant["name"], "package": variant["package"]}))
        if len(choices) != 1:
            raise ValueError("unknown or mixed selected nlohmann_json header family")
        json_inputs, json_variant = choices[0]
    entries = manifest["inputs"] + selected_json["inputs"] + json_inputs
    sources = {}
    outputs = {}
    receipts = []
    for entry in entries:
        root, relative = entry["root"], entry["path"]
        data = scoped_path(roots[root], relative).read_bytes()
        if digest(data) != entry["sha256"]:
            raise ValueError("pinned runtime notice input changed: " + root + "/" + relative)
        sources[root, relative] = data
        receipts.append({"input": root + "/" + relative, "sha256": entry["sha256"]})
        if "output" in entry:
            excerpts = []
            for start, end in entry["ranges"]:
                if not 0 <= start < end <= len(data):
                    raise ValueError("invalid bounded notice extraction range")
                excerpts.append(data[start:end])
            output = entry["output"]
            if output in outputs:
                raise ValueError("duplicate runtime notice output: " + output)
            outputs[output] = b"\n\n".join(excerpts)
    for name, font in manifest["fonts"].items():
        decoded = font_bytes(sources[font["source_root"], font["source_path"]], font)
        if digest(decoded) != font["sha256"]:
            raise ValueError("pinned embedded font identity changed: " + name)
    header = sources["lus", "include/ship/window/gui/IconsFontAwesome4.h"]
    defines = {k.decode(): v.strip().decode() for k, v in re.findall(rb"^#define\s+(\w+)\s+([^\r\n/]+)", header, re.M)}
    canonical = json.dumps(defines, sort_keys=True, separators=(",", ":")).encode()
    if len(defines) != manifest["icon_defines"]["count"] or digest(canonical) != manifest["icon_defines"]["sha256"]:
        raise ValueError("generated icon definitions differ from pinned primary header")
    receipt = {"schema": 1, "scope": manifest["scope"], "manifest_sha256": digest(manifest_bytes),
               "pins": dict(manifest["pins"], nlohmann_json=selected_json["commit"]),
               "nlohmann_json_version": args.json_version, "nlohmann_json_layout": layout,
               "nlohmann_json_variant": json_variant,
               "license_choices": manifest["license_choices"], "fonts": manifest["fonts"],
               "icon_defines": manifest["icon_defines"], "primary_material": manifest["primary_material"],
               "inputs": receipts, "outputs": {p: digest(d) for p, d in sorted(outputs.items())}}
    outputs["notice-provenance.json"] = (json.dumps(receipt, indent=2, ensure_ascii=False) + "\n").encode()
    destination = Path(args.installed or args.output)
    for relative, data in outputs.items():
        path = scoped_path(destination, relative)
        if args.check or args.installed:
            if not path.is_file() or path.read_bytes() != data:
                raise ValueError("runtime notice staging mismatch: " + relative)
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            if not path.exists() or path.read_bytes() != data:
                path.write_bytes(data)
    # The output root is exclusively owned by this extractor, not a host scan.
    if destination.is_dir():
        actual = {p.relative_to(destination).as_posix() for p in destination.rglob("*") if p.is_file()}
        if actual != set(outputs):
            raise ValueError("unexpected files in scoped runtime notice staging")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True)
    for name in ("lus", "imgui", "monocypher", "stb", "json"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--json-version", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--installed")
    args = parser.parse_args()
    try:
        prepare(args)
    except (OSError, ValueError, KeyError, TypeError, OverflowError) as error:
        print("Runtime notices: " + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
