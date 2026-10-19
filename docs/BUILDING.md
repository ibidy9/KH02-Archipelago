# Building and maintaining the project

Players should use the APWorld from Releases. These instructions are for contributors.

## Repository layout

- `worlds/kh02`: world generation, item/location rules, options, and launcher integration.
- `payload/client/kh02`: client, receipt journal, preparation, and runtime coordination.
- `payload/tools` and `payload/data`: client support code and catalogs.
- `native`: exact C/assembly source dependencies for the accepted startup and gameplay DLLs. Shared files are stored once; variant folders contain only differences.
- `poptracker/kh02-ap`: the 0.4.11 tracker, including layouts, rules, images, and artwork provenance.
- `tools`: portable offline build scripts.

Version numbers and `_candidate` suffixes in source filenames are historical. They are retained because the tested code imports or includes those names. They do not identify unsupported experiments in this export.

## Build an APWorld and tracker

Use 64-bit Python 3.13. Packaging requires only the Python standard library.

Download `kh02.apworld` from the Steam public-beta release associated with this source. From the repository root:

```powershell
py -3.13 tools/build.py import-release "C:\Downloads\kh02.apworld"
py -3.13 tools/build.py package
py -3.13 -m unittest discover -s tests -v
```

The import validates the release hash and extracts only required binary inputs into ignored `vendor/`. Outputs are `dist/kh02.apworld` and `dist/kh02-poptracker-0.4.11.zip`. These commands do not install anything or access game saves. Native binaries and game-derived assets remain those of the tested release unless explicitly replaced.

The APWorld requires Archipelago to run; standalone Python does not provide Archipelago's `Options`, `BaseClasses`, or client UI modules. The public release was tested with Archipelago 0.6.7. Install a rebuilt APWorld in a separate development installation for generation and launcher testing.

## Rebuild native code

The release was built with the project's Zig 0.16.0 Windows compiler. Supply your Zig executable explicitly:

```powershell
py -3.13 tools/build_native.py --zig "C:\Tools\zig\zig.exe"
py -3.13 tools/build.py package --runtime-dir build/runtime
```

`--variant 1` through `--variant 8` can limit compilation. Selections 1–4 are fresh Castle/WW/Forest/Depths; 5–8 are the corresponding resume selections. Startup uses v187 sources and gameplay uses v277 sources. `native/build-spec.json` records the source hashes and compilation units. The builder preserves existing output directories rather than overwriting them.

Native rebuilding is optional for Python-only changes. Newly compiled DLLs are development artifacts and need equivalent offline and live validation before publication. Compilation alone does not establish runtime compatibility. The packaging command updates the runtime catalog hashes when rebuilt DLLs are explicitly selected.

## Game-derived assets

The two cooked PAK inputs are imported from the accepted release with hashes. This repository does not redistribute extracted base-game assets, the game executable, or the historical asset-research workspace. The native and Python sources are buildable here; regenerating the cooked PAKs from original game files is not yet a standalone supported pipeline. Reuse the verified release PAKs for code changes. Changes to those assets require the supported Steam game files, suitable Unreal asset tools, and new validation.

Python's bundled SQLite fallback is also imported from the release. Its redistribution license is included in `licenses/Python-LICENSE.txt`.

## Release provenance and validation

The accepted release combines v277 native variants, v279 completed-WW routes, and the v280 client/world package. Its seeded playthrough, focused gameplay checks, and actual client shutdown check passed. `docs/release-provenance.json` identifies the source release.

The payload's historical integration contracts retain their original content for traceability. Some fields describe earlier pending tests and are superseded by this release's acceptance; they are not the current release-status document.

Source-export validation checks dependency completeness, all sixteen native builds, Python syntax, package manifests, deterministic packaging, runtime catalog consistency, and preservation of the tested gameplay code/assets. This does not substitute for live testing after functional changes.

## Publish

Push this source folder to a new GitHub repository. Create a release marked **Pre-release**, use `RELEASE-NOTES.md` for its description, and attach the accepted player downloads: `kh02.apworld`, the tracker ZIP, and optionally `START-HERE.txt`. Do not replace the accepted APWorld with a newly compiled development build just to publish the source.

Do not commit `vendor/`, build outputs, saves, seed files, logs, credentials, or personal test profiles. The repository has no remote configured and nothing has been uploaded by its preparation scripts.
