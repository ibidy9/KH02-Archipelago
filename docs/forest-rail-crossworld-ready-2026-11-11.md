Update: cross-world rail live-passed and closed; evidence research/forest-rail-crossworld-passed-20261111. Separate discovery/menu fix is now at forest-discovery-ready-2026-11-11.md. Historical preparation follows.

# Cross-world Ivy Rail retry — ready offline, not armed

Candidate: build/forest-rail-crossworld-20261111/delivery. Prior accepted same-Forest integration retained at build/forest-rail-integrated-20261111. Recovery: backups/before-forest-rail-crossworld-20261111/recovery.zip and manifest.json, member hashes verified. Installed live-passed single-Forest package remains untouched. No profiles/saves changed.

## Change
The Ivy Rail dispatch resolves the three existing Forest save discovery bits and calls menu_rail_return215. When already in Forest it uses the live-proven no-write mode1 path. From another world it calls the existing ordinary Forest return transaction, selecting Rocky Path admission. This reconstructs350 when final Darkside is completed or Path's End is discovered, otherwise323 when Rocky Path is discovered. It preserves all switches, discovery, objective/chest history and other record bytes. Native map/spawn remains Autosave_06, so entry remains before the rail. No separate milestone ledger or new story restoration timer.

The live engine still needs to establish that cross-world reload with reconstructed350 rearms the rail while leaving the landing fight and Darkside completed. Same-world story360 live evidence does not prove this exact case. This candidate must not be marked live-passed yet.

## Offline evidence
- Guarded C fixture passes same-Forest story323..360 preservation and accepted lag, permission/recheck refusals, existing retry mode behavior.
- Castle/WW/Depths departure matrix covers completed and Rocky-only Forest, verifies only both story words change. Missing Rocky discovery refuses. Both partial-write failures roll back byte-for-byte. Existing ordinary return guards and authority remain intact.
- All eight strict-warning native builds pass. Each source set differs from prior integration only in menu_forest_v149.inc and menu_return_v215.h. Same-Forest no-write header unchanged.
- Matching UE4SS host built with catalog hash allowlist; import symbol sets and exports unchanged. Package verification confirms unchanged Python/world/startup/Lua/paks; runtime and installed-mod checksums match; standard tracker retains the overlap markers. No personal tracker included.

## Live procedure — after explicit readiness only
1. Confirm game/client closed, backup installed APWorld/main.dll (current installed single-runtime trial differs from full previous delivery). Verify candidate hashes, install APWorld/matchinghost and verify complete mod.
2. resume_room.py --user-ready uses the ORIGINAL forest-live/room.apsave and generated seed. Never start this profile with empty server history. Helper uses controls-crossworld-rail, no observation expiry. Do not run it before user returns.
3. Connect ReviewForestRetr to newly printed localhost port. Launch manually after prepared, wait attachment, verify Forest RESUME core hash. LOAD SLOT1 (verified completed Forest story360 at Path's End). No new game/full clear.
4. Create controls-crossworld-rail/send-ww after active state: ordinary AP item delivery, sends WW access at most once and retains history. User waits popup, travels to WW and stands at hub save. Do not require mirrors or boss fights. Earlier world-entry machinery already tested; this is staging only.
5. From WW choose Retry Forest Ivy Rail directly. Observe native rail start and completion; expected completed landing fight remains absent. Stop at Rocky Path with control, verify visible save/Path's End destination. Walk forward past final Darkside; it must remain absent. If respawned, pause and capture, no repeat boss fight required. If absent, activate Forest ending and return to Path's End to confirm route still works; no duplicate boss/exit AP reward.
6. No slot1 overwrite. Close game then client normally. Capture logs/profile/server history. If failed, restore installed source package files from backup, never roll back saves or server history. If passed, integrate/publish only on subsequent explicit scope; no automatic GitHub action.

No server/client/game started during preparation. No installed files changed. Old and new packages remain separately recoverable.
