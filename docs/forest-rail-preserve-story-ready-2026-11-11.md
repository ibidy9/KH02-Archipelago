# Ivy Rail retry preserving Forest story — offline candidate

Not installed, armed or live-proven. Maintained source and accepted delivery unchanged. Candidate: build/forest-rail-preserve-story-20261111/delivery. Recovery backup path is in backup.txt; ZIP members independently hash-verified. Prior failure capture: research/forest-ivy-review-live-20261111.

## Finding and scope
The current focused retry uses native map/spawn dw_08_Lv_Autosave_06 and separately sets story321. Historical research says a position-only warp cannot reconstruct the challenge; it does not prove native map reload requires the story reset. This candidate keeps native reload, destination, ownership checks and fade behavior. For same-Forest retry with current story323..360, it validates a stable second snapshot then returns success WITHOUT writes to either story, flags, discovery or other game state. Both story copies retain exact values, including permitted lag. Native scripts may still change story after loading: only live evidence can resolve that.

Story is global: foreign-world retry keeps its existing destination profile. This is deliberately a scoped experiment, not a complete cross-world solution. If local preservation works, cross-world retry needs Forest milestone reconstruction rather than keeping another world's story number. No production promotion until scope is resolved.

Only Forest RESUME core v27707 changes. Matching host rebuilt with updated runtime hash allowlist. Seven other cores, Python, generator, startup, asset paks and Lua unchanged. No change to standalone Forest restart or the three other focused retry profiles.

## Offline evidence
- Actual isolated slot1 decoded from a copy with PNG/native digest validation: story360, Forest map, level53. Original container hash unchanged. Slot2 empty. Use SLOT1, do not ask user for another full clear.
- Corrected copied historical C fixture: it copied only26 of27 map characters. Candidate uses full map comparison; fixture now contains full real map.
- C tests: story323..360 retained byte-for-byte, zero writes; 350/340 and323/322 lag preserved; stale snapshot/permission rejection; existing reset/Phantom profiles and partial-write rollback; Tower/Darkside modes unchanged. Strict compiler warnings pass.
- Forest core and matching MSVC host compiled. Host import symbol sets/exports match accepted host (ordering differs). Package manifests checked; all Python compiled; exact archive delta limited to selected core/catalog/mod manifest and matching host. No new external dependency.

## Next live test, only after explicit readiness
1. Verify game/client closed. Existing installed accepted candidate is unchanged. Back up current install if changed since recorded backup. Install this candidate APWorld and matching host, verify all mod and payload hashes.
2. Run resume_room.py hidden with stdout/stderr retained. It REQUIRES original forest-live/room.apsave; do not make a fresh empty room or new seed. It uses a separate controls-preserve-story directory. Read new room.json port. Player ReviewForestRetr, seed66144286956750314311.
3. User launches after client prepared, waits attachment, LOAD SLOT1. It is completed Forest at Path's End. Confirm resume mode and native core hash from launch record before directing retry. No new game.
4. Select Retry Forest Ivy Rail. Observe correct start and rail challenge/enemies. If no rail or bad loading, stop and capture; do not improvise flags or demand unrelated travel.
5. If rail works, finish landing encounter and stop Rocky Path. Confirm visible save and Path's End destination retained. Walk forward: final Darkside must NOT respawn. If it respawns, user pauses and need not fight again. If absent, reach Path's End and verify exit still operates with no duplicated boss/exit checks.
6. Do not overwrite slot1 before evidence is captured. Normal game then client close. Preserve updated server history/profile/logs; roll back candidate installed files after a failed experiment. Never restore older saves/server history or delete profiles. No broad memory reads or legacy writer.

Current status: offline preparation complete. No server/client started, no readiness published, game never launched/closed automatically, all saves untouched. The engine may condition rail activation on321; this test establishes that instead of assuming either outcome.
