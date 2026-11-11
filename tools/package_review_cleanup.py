"""Build a private cleanup candidate atop verified RC2 binaries, without installing."""
import argparse
import ast
import io
import json
from pathlib import Path
import zipfile

from build import ROOT, archive, sha, tree

RENAMES = {
    "mickey_receipts_v171": "receipt_policy",
    "mickey_delivery_v171": "item_delivery",
    "mickey_notice_delivery_v171": "notification_delivery",
    "mickey_notifications_v171": "notification_queue",
    "mickey_shared_client_v171": "shared_client",
}


def contents(data):
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        if len(z.namelist()) != len(set(z.namelist())):
            raise ValueError("Duplicate archive paths")
        return {n: z.read(n) for n in z.namelist() if not n.endswith("/")}


class WithoutDocstrings(ast.NodeTransformer):
    def generic_visit(self, node):
        node = super().generic_visit(node)
        if isinstance(node, (ast.Module, ast.ClassDef, ast.FunctionDef, ast.AsyncFunctionDef)):
            if node.body and isinstance(node.body[0], ast.Expr):
                value = node.body[0].value
                if isinstance(value, ast.Constant) and isinstance(value.value, str):
                    node.body = node.body[1:]
        return node


def normalized_ast(data, reverse_names=False):
    source = data.decode("utf-8-sig")
    parsed = ast.parse(source)
    if reverse_names:
        reverse = {v: k for k, v in RENAMES.items()}
        for node in ast.walk(parsed):
            if isinstance(node, ast.ImportFrom) and node.module:
                node.module = ".".join(reverse.get(part, part) for part in node.module.split("."))
            elif isinstance(node, ast.Import):
                for alias in node.names:
                    alias.name = ".".join(reverse.get(part, part) for part in alias.name.split("."))
    return ast.dump(WithoutDocstrings().visit(parsed), include_attributes=False)


def package(inputs, output):
    spec = json.loads((ROOT / "tools/ue4ss-rc2-inputs.json").read_text())
    accepted = {}
    for name, digest in spec["inputs"].items():
        data = (inputs / name).read_bytes()
        if sha(data) != digest:
            raise ValueError("Accepted input mismatch: " + name)
        accepted[name] = data
    world = contents(accepted["kh02.apworld"])
    before_payload = contents(world["kh02/client-payload.zip"])
    payload = dict(before_payload)
    source_map = {}
    for name, before in before_payload.items():
        if not name.startswith("client/kh02/") or not name.endswith(".py"):
            continue
        stem = Path(name).stem
        filename = RENAMES.get(stem, stem) + ".py"
        source = ROOT / "integrations/ue4ss/client" / filename
        if not source.exists():
            source = ROOT / "payload/client/kh02" / filename
        data = source.read_bytes()
        compile(data, str(source), "exec")
        if normalized_ast(before) != normalized_ast(data, True):
            raise ValueError("Non-cosmetic client change: " + name)
        del payload[name]
        target = "client/kh02/" + filename
        payload[target] = data
        source_map[target] = source.relative_to(ROOT).as_posix()
    for name, data in list(world.items()):
        if name.endswith(".py"):
            path = ROOT / "worlds" / name
            if not path.exists():
                raise ValueError("Missing world source: " + name)
            after = path.read_bytes()
            if normalized_ast(data) != normalized_ast(after):
                raise ValueError("Unexpected world-code change: " + name)
            world[name] = after
    rule_name = "kh02/data/access-rules-v261.json"
    world[rule_name] = (ROOT / "worlds" / rule_name).read_bytes()
    packed = archive(payload)
    world["kh02/client-payload.zip"] = packed
    world["kh02/client-manifest.json"] = (json.dumps({"sha256": sha(packed), "files": {n: sha(b) for n, b in sorted(payload.items())}}, indent=2) + "\n").encode()
    mod = contents(accepted["kh02-steam-mod.zip"])
    mod["KH02-AP-INSTALL.txt"] = (ROOT / "integrations/ue4ss/PLAYER-INSTALL.txt").read_bytes()
    tracker = tree(ROOT / "poptracker/kh02-ap")
    identity = json.loads(tracker["manifest.json"])
    if identity["package_uid"] != "kh02-ap-review" or "personal" in identity["name"].lower():
        raise ValueError("Personal tracker is not a release input")
    for name, data in payload.items():
        if name.endswith(".py"):
            compile(data, name, "exec")
    unchanged = [n for n in before_payload if not n.startswith("client/kh02/") or not n.endswith(".py")]
    assert all(payload[n] == before_payload[n] for n in unchanged)
    roots = {"binaries": "Binaries/Win64/", "paks": "Content/Paks/~mods/"}
    for row in json.loads(payload["installed-mod.json"])["files"]:
        assert sha(mod[roots[row["root"]] + row["path"]]) == row["sha256"]
    for row in json.loads(payload["runtime-catalog.json"])["runtimes"]:
        for key in ("startup", "transport", "prototype", "menu"):
            assert sha(payload[row[key]["path"]]) == row[key]["sha256"]
    output.mkdir(parents=True, exist_ok=False)
    delivery = output / "delivery"
    delivery.mkdir()
    files = {"kh02.apworld": archive(world), "kh02-steam-mod.zip": archive(mod),
             "kh02-poptracker-0.4.13.zip": archive(tracker),
             "PLAYER-INSTALL.txt": mod["KH02-AP-INSTALL.txt"], "README.md": (ROOT / "README.md").read_bytes()}
    for name, data in files.items():
        (delivery / name).write_bytes(data)
        if name.endswith((".zip", ".apworld")):
            with zipfile.ZipFile(io.BytesIO(data)) as z:
                assert z.testzip() is None
    report = dict(accepted_inputs=spec["inputs"], outputs={n: sha(b) for n,b in files.items()},
                  client_ast_equal_after_module_renames=True, native_payload_unchanged=True,
                  personal_tracker_included=False, effective_source_map=source_map,
                  installed=False, armed=False, published=False)
    (output / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    package(args.inputs, args.output)
    print("Private cleanup candidate built; proven native binaries retained, nothing installed or armed.")
