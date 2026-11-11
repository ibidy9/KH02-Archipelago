# Building the UE4SS integration

The integration is still a candidate. These commands build or assemble files in
a new directory; they do not install a mod, launch KH or modify saves.

## Receipt and gameplay cores

Run `python tools/build_ue4ss_core.py --prepare-only --output <new-directory>`
to assemble all eight source sets and verify them against the accepted hashes.
To compile them, replace `--prepare-only` with `--zig <path-to-zig.exe>`.
The accepted compiler is Zig 0.16.0 targeting x86_64-windows-gnu.

The source inputs are entirely in this repository:

- `native/transport/common` contains the preserved game-specific implementation.
- `native/transport/variants` supplies each world's existing start configuration.
- `core-bridge-review.patch` contains the four engine-call changes.
- `native` in this directory supplies the UE4SS bridge interfaces and adapters.
- `core-source-manifest.json` records the accepted assembled sources.

The patch applies only at exact matching lines. The assembled source hashes must
match the accepted all-world build. There is no fuzzy patching, research-folder
lookup, process discovery, or silent fallback to legacy awards. Mixed line
endings in one accepted source are preserved for byte-level comparison.

Source reconstruction was verified for all eight variants. It does not require
another gameplay test: the result matches the tested candidate source exactly.
New binaries would still need their own hashes and explicit host approval before
packaging; do not assume rebuilding preserves PE file hashes.

## Startup and native host

`startup/unified_filename.c` uses the shared sources in `native/startup/common`
at repository root. The Critical v14 implementation is unchanged. The native
host source is in this directory's `native` folder and requires the pinned
KH-compatible UE4SS SDK and MSVC ABI used by the accepted builds.

`python tools/build_ue4ss_startup.py --zig <path-to-zig.exe> --output <new-directory>`
builds the unified startup from repository sources. Compilation and its export
set were checked against the passed startup. Use `tools/build_ue4ss_host.py` from an x64 MSVC developer environment.
Supply the SDK checkout with `--sdk`, its ImGui/Zydis dependency directory with
`--sdk-deps`, the matching UE4SS import library with `--import-library`, and
the compiler with `--cl`. Supply the assembled runtime catalog with `--catalog`
and a new directory with `--output`. The tool verifies the transport hashes and
compiles the allowlist into its copy of the host source. The offline host build
has the same imports and exports as the accepted build. The private player
package uses the live-tested binary.

`tools/package_ue4ss_candidate.py` takes an accepted base APWorld, assembled
payload, matching manual mod zip, and a new output directory. It checks the
mod inventory and runtime hashes and packages the canonical client overrides.
It preserves the generator and launcher from the base APWorld.

## Mod installation boundary

The explicit binary mod and temporary client session data are separate owners.
Both cooked paks are part of the manually installed mod. Players remove the
mod to return to vanilla. Normal session cleanup and interrupted-session recovery
are limited to the readiness records.
