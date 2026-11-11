"""Build the accepted bridge from repository sources, without research folders.

Original game-specific logic stays in native/transport. The reviewed patch
contains the four changed call sites; integrations/ue4ss/native supplies the
framework adapters. Every assembled source set must match its accepted hashes.
"""

from pathlib import Path
import argparse, hashlib, json, re, shutil, subprocess

ROOT = Path(__file__).resolve().parents[1]
INTEGRATION = ROOT / "integrations/ue4ss"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def apply_reviewed_patch(directory):
    """Strict unified patch application; no fuzzy matches or shell execution."""
    lines = (INTEGRATION / "core-bridge-review.patch").read_text(encoding="utf-8").splitlines(True)
    index = 0
    while index < len(lines):
        assert lines[index].startswith("--- original/")
        name = lines[index].strip().removeprefix("--- original/")
        assert Path(name).name == name
        index += 1
        assert lines[index].strip() == "+++ ue4ss/" + name
        index += 1
        original = (directory / name).read_text(encoding="utf-8").splitlines(True)
        output, cursor = [], 0
        while index < len(lines) and lines[index].startswith("@@ "):
            match = re.fullmatch(r"@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@.*\n", lines[index])
            assert match
            start = int(match[1]) - 1
            assert cursor <= start <= len(original)
            output.extend(original[cursor:start])
            cursor = start
            removed, added = 0, 0
            index += 1
            while index < len(lines) and not lines[index].startswith(("@@ ", "--- original/")):
                line = lines[index]
                index += 1
                assert line[0] in " +-"
                if line[0] in " -":
                    assert cursor < len(original) and original[cursor] == line[1:], name
                    cursor += 1
                    removed += 1
                if line[0] in " +":
                    output.append(line[1:])
                    added += 1
            assert removed == int(match[2] or 1) and added == int(match[4] or 1)
        output.extend(original[cursor:])
        (directory / name).write_bytes("".join(output).encode("utf-8"))


def assemble(output):
    manifest = json.loads((INTEGRATION / "core-source-manifest.json").read_text(encoding="utf-8"))
    assert manifest["version"] == 1
    output.mkdir(parents=True, exist_ok=False)
    for variant, expected in manifest["expected_variants"].items():
        directory = output / "source" / variant
        shutil.copytree(ROOT / "native/transport/common", directory)
        shutil.copytree(ROOT / "native/transport/variants" / variant, directory, dirs_exist_ok=True)
        apply_reviewed_patch(directory)
        # Match the accepted source bytes, including its retained line endings.
        for name in manifest.get("crlf_patch_outputs", []):
            path = directory / name
            data = path.read_bytes().replace(b"\n", b"\r\n")
            for line in manifest.get("lf_insertions_in_crlf_outputs", []):
                data = data.replace((line + "\r\n").encode(), (line + "\n").encode())
            path.write_bytes(data)
        for name in manifest["added_sources"]:
            assert Path(name).name == name and not (directory / name).exists()
            shutil.copy2(INTEGRATION / "native" / name, directory / name)
        # Versioned adapters appended after the original owner implementation.
        # Manifest equality below verifies the resulting complete source bytes.
        for target, adapter in manifest.get("append_sources", {}).items():
            assert Path(target).name == target and Path(adapter).name == adapter
            path = directory / target
            path.write_bytes(path.read_bytes() + (INTEGRATION / "native" / adapter).read_bytes())
        actual = {p.name: digest(p) for p in directory.iterdir() if p.is_file()}
        assert actual == expected, "Source equivalence failed: " + variant
    return manifest


def build(output, compiler):
    manifest = assemble(output)
    spec = json.loads((ROOT / "native/build-spec.json").read_text(encoding="utf-8"))
    results = []
    for variant in manifest["expected_variants"]:
        source = output / "source" / variant
        target = output / "runtime" / variant / "Kh02FreshTransport.dll"
        target.parent.mkdir(parents=True)
        flags = [
            "KH02_STRUCTURED_CLEAR",
            "KH02_GAME_GRANT_CALLS=1",
            "KH02_COSMETIC_GAME_CALLS=1",
            "KH02_REUSABLE_SAVE_PIPELINE",
            "KH02_FRESH_LIFECYCLE",
            "KH02_GRANT_OWNER_LIVE",
            "KH02_FRESH_TRANSPORT_RUNTIME",
            "KH02_SAVE_TRANSPORT_DIAGNOSTIC",
            "KH02_SAVE_HEADER_BINDING",
        ]
        if int(variant[-2:]) <= 4:
            flags.append("KH02_FRESH_SAVE_HARNESS")
        units = spec["transport"]["units"] + [
            "reflected_grant.c",
            "ue4ss_transport_bridge.c",
            "registration_transport.c",
        ]
        command = [
            str(compiler),
            "cc",
            "-target",
            "x86_64-windows-gnu",
            "-O2",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-shared",
        ]
        command += ["-D" + flag for flag in flags] + [str(source / name) for name in units]
        command += ["-o", str(target), "-lkernel32", "-lbcrypt", "-luser32"]
        result = subprocess.run(command, capture_output=True, text=True)
        (output / (variant + ".log")).write_bytes((result.stdout + result.stderr).encode())
        if result.returncode:
            raise RuntimeError("Build failed; see " + str(output / (variant + ".log")))
        results.append(dict(variant=variant, sha256=digest(target), command=command))
    (output / "build.json").write_bytes(
        json.dumps(dict(variants=results, installed=False, armed=False), indent=2).encode()
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--zig", type=Path)
    parser.add_argument("--prepare-only", action="store_true")
    args = parser.parse_args()
    if args.prepare_only:
        assemble(args.output.resolve())
        print("All eight source sets match accepted hashes; no binaries built or installed.")
    else:
        if args.zig is None:
            parser.error("--zig is required for compilation")
        build(args.output.resolve(), args.zig.resolve(strict=True))
