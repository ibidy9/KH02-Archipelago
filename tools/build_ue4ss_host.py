"""Build the UE4SS host in an x64 MSVC developer environment."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("output", "sdk", "sdk-deps", "import-library", "catalog", "cl"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    source = output / "native"
    shutil.copytree(ROOT / "integrations/ue4ss/native", source)
    catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
    hashes = []
    for runtime in catalog["runtimes"]:
        entry = runtime["transport"]
        actual = hashlib.sha256((args.catalog.parent / entry["path"]).read_bytes()).hexdigest()
        if actual != entry["sha256"]:
            raise ValueError("Transport differs from catalog for " + runtime["version"])
        if actual not in hashes:
            hashes.append(actual)
    if not hashes:
        raise ValueError("Empty transport catalog")
    approved = ",".join('"' + value + '"' for value in hashes)
    (source / "approved_core_hashes.hpp").write_text(
        "#pragma once\ninline constexpr const char* approved_core_hashes[]={" + approved + "};\n",
        encoding="ascii",
    )
    sdk = args.sdk.resolve(strict=True)
    unreal = sdk / "deps/first/Unreal"
    dependencies = args.sdk_deps.resolve(strict=True)
    includes = [
        sdk / "UE4SS/include",
        *sorted((sdk / "deps/first").glob("*/include")),
        unreal / "generated_include",
        dependencies / "imgui",
        dependencies / "ImGuiColorTextEdit",
        unreal / "include/Unreal/Core",
        dependencies / "zydis/include",
        dependencies / "zycore/include",
        unreal / "include/Unreal",
    ]
    flags = [
        "/nologo",
        "/c",
        "/MD",
        "/EHsc",
        "/std:c++latest",
        "/Zc:__cplusplus",
        "/Zc:preprocessor",
        "/DUE_GAME",
        "/DUE_BUILD_SHIPPING",
        "/DPLATFORM_WINDOWS",
        "/DPLATFORM_MICROSOFT",
        "/DUNICODE",
        "/D_UNICODE",
        "/DOVERRIDE_PLATFORM_HEADER_NAME=Windows",
        "/DUBT_COMPILED_PLATFORM=Win64",
        "/O2",
        "/wd4005",
        "/DNDEBUG",
        "/D_ITERATOR_DEBUG_LEVEL=0",
    ]
    cl = args.cl.resolve(strict=True)
    subprocess.run(
        [cl, *flags, *["/I" + str(p) for p in includes], source / "host.cpp", "/Fohost.obj"],
        cwd=output,
        check=True,
    )
    subprocess.run(
        [
            cl.with_name("link.exe"),
            "/nologo",
            "/DLL",
            "/OUT:main.dll",
            "host.obj",
            args.import_library.resolve(strict=True),
            "user32.lib",
            "kernel32.lib",
            "bcrypt.lib",
        ],
        cwd=output,
        check=True,
    )


if __name__ == "__main__":
    main()
