# Mines message r2 — ready offline, NOT armed

Supersedes docs/mines-message-ready-2026-11-05.md for the next test. The passed automatic entry gate remains unchanged.

r1 refused before calling GetDistanceTo: runtime ParmsSize12 versus expected16. C++ buffer tail padding was mistaken for reflected parameter extent. The banner had the same assumption (buffer72 versus last field ending65). No distance or banner call executed in r1. This was a preparation mistake.

r2 uses observed distance extent12 with padded16-byte backing storage. The information call accepts a reflected extent covering the last field and fitting its backing buffer (65..72), with exact flags/count/return and individual property names/types/offsets still checked. All four TresLocText fields are verified. Full contract preflight now happens at the hub BEFORE distance/proximity, logging CONTRACT_READY; no need to walk to discover a metadata refusal. Static input lifetime and native copy behavior remain as audited in r1. No new core, receipt, save, pak, or entry-gate changes.

Host compile/import checks, package validation, unchanged cooldown/hysteresis policy test and real offline AP send/Sync passed. All payload files other than host manifest are identical to r1 (excluding local Python bytecode caches, not packaged). Decoded a COPY of isolated MinesMessage save and read journals readonly: resume_records accepts slot1 for seed12420762263808334479 with empty item history. Source save unchanged. See resume-preflight.json.

r1 capture is named in build/ue4ss-mines-message-20261105/active-capture.txt. Failed candidate backup under backups/mines-message-r1-*; passed gate backup remains backups/before-mines-message-20261105. Installed RC1 restored/hash-verified; local helper stopped. Playthrough saves untouched.

## Next live session — only on explicit readiness
Back up current installed files and test profile. Install build/ue4ss-mines-message-r2-20261105/package/kh02.apworld and host/main.dll; verify installed-mod manifest. Start tools/mines_message_r2_room_20261105.py live --user-ready hidden with stdout/stderr/PID capture. SAME seed, player MinesMessage, new address in r2/live-room.json. Do not generate another seed.
User connects a fresh client, waits prepared, launches normally. Confirm attached at title, then LOAD SLOT1, NOT New Game. At hub verify active, gate APPLIED, and full CONTRACT_READY before requesting movement.
Approach Mines: expect Doubleflight required banner, movement/menu usable, no entry prompt. Stay nearby to check no spam; move away/reapproach after15 seconds. Check another intact mirror prompt without entering.
Touch r2/live-controls/send-doubleflight once. Expect actual AP popup, ability and normal prompt, no new locked reminder. No save/reload/traversal repeat needed unless new evidence warrants it.
After normal user closure, archive logs/profile, stop helper, restore installed RC1 by hashes. Never restore profiles automatically.

Remaining: banner appearance/timing/radius are not live-proven. Earlier closure acknowledgement failure and fresh Phantom1/2 reporting witness remain separate outstanding work. Nothing is installed or armed for r2.
