"""Package an accepted UE4SS payload and its matching manual mod, offline."""

import argparse
import configparser
import io
import json
import re
from pathlib import Path
import zipfile
from build import ROOT, archive, sha, tree


def package(base, payload_root, manual_mod, output):
    payload = tree(payload_root)
    for path in (ROOT / "integrations/ue4ss/client").glob("*.py"):
        payload["client/kh02/" + path.name] = path.read_bytes()
    catalog = json.loads(payload["runtime-catalog.json"])
    for runtime in catalog["runtimes"]:
        for key in ("startup", "transport", "prototype", "menu"):
            entry = runtime[key]
            if sha(payload[entry["path"]]) != entry["sha256"]:
                raise ValueError("Runtime catalog mismatch: " + entry["path"])
    # Build inputs retain historical binaries for recovery. Players need only
    # the entries selectable by this catalog. Identical startup DLLs can share
    # a path; distinct future startup builds keep their own paths.
    startup_paths = {}
    selected = set()
    for runtime in catalog["runtimes"]:
        startup = runtime["startup"]
        startup["path"] = startup_paths.setdefault(startup["sha256"], startup["path"])
        selected.update(
            runtime[key]["path"] for key in ("startup", "transport", "prototype", "menu")
        )
    payload = {
        name: data
        for name, data in payload.items()
        if not name.startswith("runtime/") or not name.endswith(".dll") or name in selected
    }
    payload["runtime-catalog.json"] = (json.dumps(catalog, indent=2) + "\n").encode()
    mod = manual_mod.read_bytes()
    roots = {"binaries": "Binaries/Win64/", "paks": "Content/Paks/~mods/"}
    manifest = json.loads(payload["installed-mod.json"])
    expected = {roots[row["root"]] + row["path"]: row["sha256"] for row in manifest["files"]}
    with zipfile.ZipFile(io.BytesIO(mod)) as source:
        if len(source.namelist()) != len(expected) or set(source.namelist()) != expected.keys():
            raise ValueError("Mod archive inventory differs from the client manifest")
        for name, digest in expected.items():
            if sha(source.read(name)) != digest:
                raise ValueError("Mod archive mismatch: " + name)
        # The lab reused an existing Microsoft controller DLL. It belongs to the
        # DirectX installation and must not become an AP install/uninstall target.
        system_file = "Binaries/Win64/xinput1_3.dll"
        mod_files = {name: source.read(name) for name in expected if name != system_file}
    settings = configparser.ConfigParser(interpolation=None)
    ini_path = "Binaries/Win64/UE4SS-settings.ini"
    # The fork's IniParser rejects adjacent section headers. Removing the lab's
    # ModsFolderPath entry left an empty leading Overrides section in older inputs.
    mod_files[ini_path] = re.sub(rb"\A\[Overrides\]\r?\n(?=\[General\])", b"", mod_files[ini_path])
    settings.read_string(mod_files[ini_path].decode("utf-8-sig"))
    if settings.get("Overrides", "ModsFolderPath", fallback="").strip():
        raise ValueError("Player packages must use the UE4SS default Mods directory")
    for name in (
        "Mods/mods.txt",
        "Mods/KH02ReceiptHost/dlls/main.dll",
        "Mods/KH02ReceiptHost/Scripts/main.lua",
    ):
        if "Binaries/Win64/" + name not in mod_files:
            raise ValueError("Missing file in default UE4SS mod layout: " + name)
    manifest["files"] = [
        row for row in manifest["files"] if roots[row["root"]] + row["path"] != system_file
    ]
    for row in manifest["files"]:
        if roots[row["root"]] + row["path"] == ini_path:
            row["sha256"] = sha(mod_files[ini_path])
    payload["installed-mod.json"] = (json.dumps(manifest, indent=2) + "\n").encode()
    mod_files["KH02-AP-INSTALL.txt"] = (ROOT / "integrations/ue4ss/PLAYER-INSTALL.txt").read_bytes()
    mod_files["KH02-AP-UE4SS-LICENSE.txt"] = (
        ROOT / "integrations/ue4ss/UE4SS-LICENSE.txt"
    ).read_bytes()
    mod = archive(mod_files)
    # Preserve the accepted generator, YAML options and launcher. Replace only
    # its bundled client and the hash manifest consumed by launcher.unpack().
    with zipfile.ZipFile(base) as source:
        world = {n: source.read(n) for n in source.namelist() if not n.endswith("/")}
    if (
        not {"kh02/launcher.py", "kh02/client-manifest.json", "kh02/client-payload.zip"}
        <= world.keys()
    ):
        raise ValueError("Expected a KH 0.2 APWorld")
    packed = archive(payload)
    world["kh02/client-payload.zip"] = packed
    world["kh02/client-manifest.json"] = json.dumps(
        {
            "sha256": sha(packed),
            "files": {name: sha(data) for name, data in sorted(payload.items())},
        },
        indent=2,
    ).encode()
    output.mkdir(parents=True, exist_ok=False)
    (output / "kh02.apworld").write_bytes(archive(world))
    (output / "kh02-steam-mod.zip").write_bytes(mod)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("base", "payload", "mod", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    package(args.base, args.payload, args.mod, args.output)
