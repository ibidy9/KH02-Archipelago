"""Consolidate final live-passed Steam inputs, without rebuilding, installing or accessing saves."""

import argparse
import ast
import io
import json
from pathlib import Path
import zipfile
from build import ROOT, archive, sha


def contents(data):
    with zipfile.ZipFile(io.BytesIO(data)) as source:
        names = source.namelist()
        if len(names) != len(set(names)):
            raise ValueError("Duplicate archive members")
        return {n: source.read(n) for n in names if not n.endswith("/")}


def consolidate(inputs, output):
    spec = json.loads((ROOT / "tools/steam-release-inputs-20261110.json").read_text(encoding="utf-8"))
    accepted = {}
    for name, digest in spec["inputs"].items():
        data = (inputs / name).read_bytes()
        if sha(data) != digest:
            raise ValueError("Accepted input hash mismatch: " + name)
        accepted[name] = data
    world = contents(accepted["kh02.apworld"])
    payload = contents(world["kh02/client-payload.zip"])
    manifest = json.loads(world["kh02/client-manifest.json"])
    if sha(world["kh02/client-payload.zip"]) != manifest["sha256"]:
        raise ValueError("Payload hash mismatch")
    if {n: sha(d) for n, d in payload.items()} != manifest["files"]:
        raise ValueError("Payload inventory mismatch")

    source_map = {}
    formatting_only = []
    for name, data in payload.items():
        if not name.endswith((".py", ".lua")):
            continue
        if name.endswith(".py"):
            compile(data, name, "exec")  # Detect source-encoding errors too.
        source = ROOT / "payload" / name
        override = ROOT / "integrations/ue4ss/client" / Path(name).name
        if name.startswith("client/kh02/") and override.exists():
            source = override
        if name.startswith("sqlite-support/sqlite3/"):
            source_map[name] = "Third-party CPython SQLite support; licenses/Python-LICENSE.txt"
            continue
        if not source.exists():
            raise ValueError("Missing effective source: " + name)
        if source.read_bytes() != data:
            if name != "tools/save_filename_support.py" or ast.dump(ast.parse(data)) != ast.dump(ast.parse(source.read_bytes())):
                raise ValueError("Packaged source differs from effective repository source: " + name)
            formatting_only.append(name)
        source_map[name] = source.relative_to(ROOT).as_posix()
    for source in (ROOT / "worlds/kh02").rglob("*"):
        if source.suffix not in (".py", ".json") or "__pycache__" in source.parts:
            continue
        name = "kh02/" + source.relative_to(ROOT / "worlds/kh02").as_posix()
        if world.get(name) != source.read_bytes():
            raise ValueError("World source mismatch: " + name)

    mod = contents(accepted["kh02-steam-mod.zip"])
    roots = {"binaries": "Binaries/Win64/", "paks": "Content/Paks/~mods/"}
    for row in json.loads(payload["installed-mod.json"])["files"]:
        name = roots[row["root"]] + row["path"]
        if sha(mod[name]) != row["sha256"]:
            raise ValueError("Mod mismatch: " + name)
    for row in json.loads(payload["runtime-catalog.json"])["runtimes"]:
        for key in ("startup", "transport", "prototype", "menu"):
            entry = row[key]
            if sha(payload[entry["path"]]) != entry["sha256"]:
                raise ValueError("Runtime catalog mismatch")

    tracker_name = "kh02-poptracker-0.4.13.zip"
    tracker = contents(accepted[tracker_name])
    tracker_manifest = json.loads(tracker["manifest.json"])
    if (
        tracker_manifest["package_version"] != "0.4.13"
        or tracker_manifest["package_uid"] != "kh02-ap-review"
        or "personal" in tracker_manifest["name"].lower()
    ):
        raise ValueError("Only the standard 0.4.13 tracker belongs in delivery")
    rules = (ROOT / "poptracker/kh02-ap/scripts/shared-access-rules.lua").read_bytes()
    if tracker["scripts/shared-access-rules.lua"] != rules:
        raise ValueError("Tracker rules differ from source")
    for name, data in tracker.items():
        source = ROOT / "poptracker/kh02-ap" / name
        if not source.is_file() or source.read_bytes() != data:
            raise ValueError("Tracker source mismatch: " + name)
        if name.endswith(".json"):
            json.loads(data)

    # Only the human-readable installation sheet changes inside the passed mod.
    mod["KH02-AP-INSTALL.txt"] = (ROOT / "integrations/ue4ss/PLAYER-INSTALL.txt").read_bytes()
    delivery = {
        "kh02.apworld": accepted["kh02.apworld"],
        "kh02-steam-mod.zip": archive(mod),
        tracker_name: accepted[tracker_name],
        "PLAYER-INSTALL.txt": mod["KH02-AP-INSTALL.txt"],
        "README.md": (ROOT / "README.md").read_bytes(),
    }
    output.mkdir(parents=True, exist_ok=False)
    player = output / "delivery"
    player.mkdir()
    for name, data in delivery.items():
        (player / name).write_bytes(data)
    report = dict(
        inputs=spec["inputs"],
        outputs={n: sha(d) for n, d in delivery.items()},
        changed_mod_members=["KH02-AP-INSTALL.txt"],
        apworld_byte_identical=True,
        native_files_byte_identical=True,
        personal_tracker_included=False,
        effective_source_map=source_map,
        formatting_only_source_differences=formatting_only,
    )
    (output / "verification.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    consolidate(args.inputs, args.output)
    print("Consolidated accepted Steam delivery; no game files or saves changed.")
