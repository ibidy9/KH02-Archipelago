# System SQLite portability correction

## Established failure

The second-computer diagnostic returned a validated ARM rejection (status 2, active 0) in 0.01 seconds. This was not a timeout. Its system SQLite reports 3.51.1; the development computer reports 3.43.2. The native journal loader required an exact `sqlite3_libversion()` string of `3.43.2`, refusing other Windows-serviced versions before opening the receipt journal. This is a portability defect. The two public tester archives do not establish their SQLite versions.

An isolated offline reproduction confirms the old journal rejects an initialized temporary database with upstream SQLite 3.51.1, while the updated journal accepts the same schema. This establishes the version gate as a sufficient cause of journal refusal. A successful game attachment on the second computer remains a required live test; it could reveal another failure later in initialization.

## Change

The native journal still loads `winsqlite3.dll` from System32 and resolves every SQLite export used. It no longer requires an exact release string. On the opened connection, read-only probes check JSON iteration, nested JSON extraction preserving unsigned 64-bit receipt text, and conversion of extracted scope text to blob bytes. Existing FULL synchronization, schema marker, database integrity, digest, cross-record link, transactional, thread ownership, and duplicate/conflict checks remain intact. No database is created or migrated by native open.

All eight transport variants are rebuilt. Accepted startup DLLs and both PAKs are reused byte-for-byte. The candidate also retains the prior diagnostic error reporting and 30-second one-shot ARM allowance, plus the generation helper compatibility fix. No live-installed files, development saves, or public release assets were modified.

## Validation

- 14 native journal tests pass on the system SQLite 3.43.2.
- The same 14 tests pass on upstream SQLite 3.51.1 in isolated test-only builds whose sole extra adaptation is an explicit library path. This is not the Windows-provided 3.51.1 binary and does not replace testing that binary on the second computer.
- Tests cover complete/duplicate/conflicting receipt publications, corruption, missing links, interrupted commits, database contention, unsigned receipt IDs, Unicode scopes, and close behavior.
- All eight packaged transport builds open and close temporary journals at Unicode paths without modifying their bytes, refuse missing databases without creating them, and reject an invalid schema marker.
- All 12 source/package/diagnostic tests pass with rebuilt-runtime checks enabled.
- The old-vs-new offline 3.51.1 comparison demonstrates rejection followed by acceptance.

## Environment audit

Inspection covered shipped Python, PowerShell, native startup/transport source, runtime selection, and bundled support dependencies. No hardcoded developer username, drive, home directory, Steam library root, account ID, or Windows build-number requirement was found in the inspected runtime code. Steam installation/account information comes from Steam's registry/configuration and AP paths from the current user's local application-data folder.

The remaining narrow requirements have specific purposes:

- The Steam executable hash, native instruction signatures, offsets, and relevant structure layouts identify the supported game build. Keep these checks; using another executable requires compatibility work.
- Runtime/configuration hashes, protocol versions, journal schema, and seed/profile identities protect consistency, not host identity.
- Native modules require Windows x64. Startup paths are bounded by the current protocol (1024 UTF-16 units; journal path up to 1023 UTF-8 bytes). Very long installations remain a documented implementation limit.
- The optional bundled Python SQLite fallback requires 64-bit Python 3.13 because its compiled extension is tied to that ABI. It is only used when Archipelago lacks SQLite support. This is distinct from the Windows native journal library and must not be relaxed without another compatible extension.
- PowerShell is used for a bounded game-process inventory and the bundled save decoder. No standalone development compiler, Python installation, developer checkout, or per-user external tool path is required by players.
- A live signed-in Steam account and one unambiguous supported installation remain deliberate discovery requirements. Another running Steam game is excluded from KH process discovery.

These findings are a targeted portability audit, not a complete security audit or a claim of compatibility with every Windows/SQLite version.

## Recovery and next live test

Workspace backup `backups/sqlite-portability-before-20261031` preserves source, configuration, accepted public binaries, and the earlier diagnostic APWorld, with hashes and restore instructions. Builds and evidence live in `build/sqlite-portability-20261031`. No test is armed.

On the second computer, finish the previous client's cleanup and close both applications. Preserve its existing profile by renaming `profiles` to a new unused name such as `profiles-before-sqlite-fix`; never delete or adopt its unassociated save. Install the candidate APWorld, reopen the Launcher, reconnect to the same test room, and launch 0.2 only after preparation. Wait at the title screen. If attachment succeeds, record the log and then verify a fresh game reaches active AP control; do not load a vanilla save. If it fails, retain `attachment-failure.json` and the complete client log and close normally for cleanup.
