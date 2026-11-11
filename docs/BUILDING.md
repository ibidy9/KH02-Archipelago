# Building and checking the Steam UE4SS release

## Final live-passed consolidation

The final source includes same-world and cross-world Forest Ivy Rail retry story preservation, plus consistent earlier-save availability when a later Forest save has been discovered. See `forest-discovery-passed-2026-11-10.md` and the preceding Forest retry records.

Use the three accepted release archives pinned in `tools/steam-release-inputs-20261110.json`. In the development workspace these are preserved under `build/forest-discovery-20261111/delivery`.

```powershell
py -3 -X utf8 tools/consolidate_steam_release.py --inputs <accepted-input-directory> --output <new-output-directory>
py -3 -X utf8 tools/export_tracker_rules.py --check
py -3 -X utf8 -m unittest discover -s tests
py -3 -X utf8 -m unittest discover -s integrations/ue4ss/tests
```

The consolidator verifies effective client sources, generator sources, payload inventory, runtime catalogs, installed mod hashes and standard tracker contents against this repository. It preserves the accepted APWorld and every executable/pak/config member of the mod. Only the mod's installation sheet changes. No installation, test arming or publication occurs.

The formatted save-filename helper is the sole allowed source-byte difference; its Python AST must match exactly and the report identifies it.

The output separates player downloads from a maintainer verification report. The personal tracker is excluded. Hashes are verification records, not a security audit.

For native rebuilding use the instructions below and the current source manifests. A newly compiled DLL is a new artifact, not automatically the live-passed DLL. The generated host allowlist must match the packaged eight core variants.

`consolidate_ue4ss_release.py`, `ue4ss-rc2-inputs.json` and `package_review_cleanup.py` reproduce earlier stages. They deliberately pin older native binaries and must not be used to assemble this final release.

## Source layout and overlays

- `worlds/kh02`: generation, settings, access rules, AP launcher entry.
- `payload/client/kh02`: shared client implementation. Historical versioned module names are still active imports, not disposable files.
- `integrations/ue4ss/client`: replacement modules copied over identically named payload modules during UE4SS packaging. These are the effective sources for those names.
- `integrations/ue4ss/Scripts` and `native`: Lua lifecycle observations and native UE4SS host.
- `native/transport`: preserved game-specific gameplay/receipt core.
- `native/startup` and `integrations/ue4ss/startup`: early save-isolation and Critical startup code.
- `poptracker/kh02-ap`: standard 0.4.13 source, layouts, rules and artwork provenance.

The generated `verification.json` maps packaged Python modules to their effective source paths. Four CPython SQLite support files are third-party release inputs, covered by `licenses/Python-LICENSE.txt`.

## Native changes

Use [the UE4SS build instructions](../integrations/ue4ss/BUILDING.md) for core source reconstruction, native host and startup builds. Exact source reconstruction can be checked offline. Recompiled binaries are new development artifacts and require their own validation; do not substitute them for accepted binaries simply to make a release.

`tools/package_ue4ss_candidate.py` is the lower-level development packager for new candidates. It applies the UE4SS client overlay. It preserves the generator from its supplied base APWorld: if world logic changes, that base must be rebuilt and checked as well.

## Historical build tools

`tools/build.py import-release/package`, `tools/release-inputs.json`, `tools/build_native.py`, and older source-export documents describe the pre-UE4SS public beta. They remain for recovery and provenance. Their output is not a substitute for the current UE4SS delivery. Do not run the legacy package command expecting it to apply the manual-mod client overlay.

The cooked paks remain accepted game-derived inputs. There is no standalone source-only pipeline for recreating them without the supported game and asset tools. Likewise, UE4SS and CPython retain third-party build/license requirements. Source availability does not imply all binary inputs are original project code.

Never commit player saves, seed files, profiles, passwords, crash dumps or private captures. No publication is performed by these tools.
