# Final Steam UE4SS release consolidation — 2026-11-10

The player delivery is ready for publication; it has not been published by this preparation step.

The accepted input hashes are pinned in `tools/steam-release-inputs-20261110.json`. `tools/consolidate_steam_release.py` preserves the tested APWorld, native code, paks and configuration. Only the mod installation text is updated. The standard PopTracker is 0.4.13; the personal version is excluded.

Final live coverage includes the Wayfinder-only boss-rush mirror, same-world and cross-world Forest Ivy Rail retry without replaying completed story, and earlier Forest save/restart access inferred from later discoveries. The final closed session confirmed ordinary return control, retained menu entries, unchanged AP history and successful shutdown cleanup. See the linked dated pass records in this folder.

Offline consolidation checks passed: effective client source and generator comparison, payload and installed-mod inventories, all eight reconstructed native source sets matching the tested build, exact host-source comparison, tracker source comparison, and shared-rule export check. The Python suites passed 48 tests with one optional test skipped.

One packaged helper, `payload/tools/save_filename_support.py`, retains pre-formatting whitespace. Its parsed Python syntax tree exactly matches the formatted repository source. This exception is narrowly checked and recorded in verification; no executable behavior was changed to prepare this release.

The maintainer's main-path code walkthrough is recorded in `CODE-WALKTHROUGH.md`; `CODE-REVIEW-CHEATSHEET.md` is the quick reference. This is not a claim of exhaustive review of every helper/dependency or a second complete seeded playthrough of the final UE4SS package.

Private captures, profiles, test seeds, caches, experiments, third-party development trees and the personal tracker are excluded from the source snapshot. Required licenses and asset attribution remain included. The source snapshot is separate from player downloads. Internal verification hashes are not an additional required player download.

No game files or saves were changed, no test was armed, and nothing was pushed to GitHub during consolidation.
