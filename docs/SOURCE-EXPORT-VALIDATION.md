# Source export validation — 2026-10-19

The export was checked independently of the development workspace's installed game and saves.

- All eight startup DLLs and all eight gameplay DLLs compiled from the exported source dependencies with Zig.
- Native source hashes match the selected release source snapshots.
- Repackaging with the verified release inputs preserved every world source file and every embedded client payload member byte for byte. ZIP container bytes differ because the build uses deterministic archive metadata.
- Package manifest and runtime-catalog hashes passed, including all eight runtime selections.
- Python syntax, deterministic archive ordering, and rejection of an incorrect bootstrap release passed.
- Tracker images, scripts, layouts, locations, and item data match the tested 0.4.11 pack. Only its README and two artwork-provenance files changed: stale documentation and private local source paths were removed.
- The publishable source file list was checked for development account paths, private credentials, saves, logs, and executable/build artifacts. None were included.

The accepted APWorld was not replaced by the recompiled binaries. Native compilation is a source-completeness check, not a new live gameplay acceptance. No game was launched, no save was changed, and no live test was armed during this export.
