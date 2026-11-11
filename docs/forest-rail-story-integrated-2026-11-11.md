# Forest Ivy Rail story preservation — integrated 2026-11-11

## Live result
Passed capture: `research/forest-rail-preserve-story-passed-20261111`. Same isolated seed66144286956750314311, completed Forest slot1 (verified saved story360). The candidate replayed the rail normally, did not replay the completed landing fight, left the final Darkside defeated, and allowed the Forest ending to return the user to Path's End. AP history stayed unchanged throughout. Game-first/client-second shutdown completed normally. No save was overwritten by tooling.

This supersedes the earlier failed behavior in `forest-ivy-review-failed-2026-11-10.md`. No new full Forest clear was needed.

## Integration
`native/transport/common/menu_forest_v149.h`: only mode1, when already in Forest with story323..360, now validates the second stable snapshot and returns success without writes. Both story copies, completion flags, discovery and objective data remain untouched. Existing native Autosave_06 map reload/spawn reconstructs the rail. Other reset/retry modes retain their prior behavior. Permission loss or changed state refuses before any writes.

All eight source manifests updated, all eight runtime variants compiled with strict warnings, maintained `integrations/ue4ss/tests/forest_rail_story_test.c` passes preservation/lag/refusal/rollback/other-mode checks. The historical fixture's26-byte map copy was corrected to the complete map name. Each assembled variant differs from the prior formatted source in that one header only.

Private full delivery: `build/forest-rail-integrated-20261111/delivery`. The exact live-passed Forest resume core is retained. Other seven cores carry the same compiled fix but have not received separate live tests. Host's only intended behavior change is its approved core hash list; imports/exports verified unchanged. Python, world rules, startup, Lua, paks and tracker remain byte-identical to the previous cleanup candidate. All payload/catalog/mod hashes and Python syntax verified. Full integrated package not installed or published. The single-runtime live-tested package remains installed with its existing backup.

Recovery: `backups/before-forest-rail-integration-20261111` contains verified source/manifests and passed delivery; earlier installed binary/config/pak recovery is linked in `build/forest-rail-preserve-story-20261111/backup.txt`. Never restore old saves or server histories as package rollback.

## Scope still explicit
Story is shared between worlds. A rail retry selected FROM another world still uses the original Forest staging behavior; this integration does not keep a foreign-world story value. Preserving completed Forest progression for cross-world rail entry remains unresolved. Do not describe this as an all-context retry fix or silently mark cross-world behavior live-proven. No new test armed.
