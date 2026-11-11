"""Build unified startup from repository sources; never install or launch KH."""

from pathlib import Path
import argparse, hashlib, json, subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--zig", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    common = ROOT / "native/startup/common"
    sources = [ROOT / "integrations/ue4ss/startup/unified_filename.c"]
    sources += [
        common / (name + ".c")
        for name in (
            "startup_input",
            "startup_gate",
            "path_route",
            "provider_install",
            "resume_authorization",
            "critical_menu_startup_v14",
        )
    ]
    target = output / "dinput8.dll"
    command = [
        str(args.zig.resolve(strict=True)),
        "cc",
        "-target",
        "x86_64-windows-gnu",
        "-O2",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I" + str(common),
        "-shared",
        "-DKH02_FILENAME_LIVE",
        "-DKH02_FRESH_STORAGE_AUTH",
    ]
    command += [str(p) for p in sources] + [
        "-o",
        str(target),
        "-Wl,--out-implib," + str(output / "libdinput8.a"),
    ]
    result = subprocess.run(command, capture_output=True, text=True)
    (output / "build.log").write_bytes((result.stdout + result.stderr).encode())
    if result.returncode:
        raise RuntimeError("Build failed; see " + str(output / "build.log"))
    digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    inputs = set(sources) | {p for p in common.iterdir() if p.is_file()}
    evidence = dict(
        command=command,
        binary_sha256=digest(target),
        sources={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(inputs)},
        installed=False,
        armed=False,
        live_proven=False,
    )
    (output / "build.json").write_bytes(json.dumps(evidence, indent=2).encode())
    print("Unified startup compiled; not installed or armed.")


if __name__ == "__main__":
    main()
