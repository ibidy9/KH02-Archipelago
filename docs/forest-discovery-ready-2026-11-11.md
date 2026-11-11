Update: live-passed and closed November10 local time; see forest-discovery-passed-2026-11-10.md. Original preparation follows.

# Forest earlier-save reachability — offline candidate

Candidate build/forest-discovery-20261111/delivery; not installed or armed. Baseline is the live-passed cross-world rail package. Recovery backup backups/before-forest-discovery-20261111 includes previous maintained source/manifests and baseline delivery. Installed code unchanged; see prior live-installation.json for binary/config/pak recovery. No save data rewritten.

## Behavior
Forest later discovery implies earlier reachability: Path's End -> Rocky Path + Uncertain Path; Rocky Path -> Uncertain Path. forest_discovery.h implements a read-only effective mask using dynamically resolved native bits. No persistent bits are invented, and unrelated world bits are unchanged.

menu_visibility_v151.inc uses this same effective view for Travel/Restart/Uncertain Path and Rocky/End availability. It deliberately does not require the raw native first-save predicate when a later save proves reachability, so both native visibility callbacks agree. menu_return_v215.h admits first/earlier destinations through that same predicate, preserving raw dual-copy agreement and the existing transactional story reconstruction. No change to restart clearing, world ownership gates, rail destination or receipt logic.

## Offline checks
Maintained C test covers all8 discovery subsets and all3 destinations; expected earlier-only implication; no discovery/flag/objective changes; no later unlock from first alone; unrelated bits retained; genuine restart's clearing removes implied later access. Existing same-Forest rail preservation, cross-world completed/partial milestones, stale state/permission refusal and partial-write rollback tests continue passing.

All eight cores and matching hash-allowlisted host are built separately. Package verification requires only forest_discovery.h, menu_visibility_v151.inc and menu_return_v215.h source deltas relative to previous candidate; no Python, generator, startup, Lua, asset or tracker change. This is not live proof of menu rendering/selection.

## Final focused live check — only after readiness
1. Game/client closed; back up currently installed APWorld/main.dll, install candidate pair and verify complete inventory. Do not touch saves. Normal client/server preparation; use resume_room.py --user-ready with original forest-live/room.apsave. It refuses missing retained server history. New controls-forest-discovery avoids retired stop flags.
2. Player ReviewForestRetr, seed66144286956750314311. Load SLOT1 (completed Forest, raw discovery0x18, first save missing). WW access is already in retained AP history; do not send again unless actual authenticated history disproves this. Wait active/popups. No new game.
3. Go to WW hub. Confirm Forest: Uncertain Path, Rocky Path and Path's End plus Restart Forest. No Travel to Forest. Retry entries remain present.
4. Select Forest: Uncertain Path, confirm actual arrival/control/save point. This must use normal return and retain completed progression, not the opening/restart. Check menu still retains Rocky/End and Restart. No boss replay required.
5. Normal close game then client. Preserve capture and history. No slot1 overwrite required. If failure, restore prior package only, never old saves/history.

Cross-world rail itself passed in research/forest-rail-crossworld-passed-20261111 with no repeated landing fight or Darkside, working ending and unchanged AP history. This candidate addresses the separate observed menu issue. No new unrelated testing is planned.
