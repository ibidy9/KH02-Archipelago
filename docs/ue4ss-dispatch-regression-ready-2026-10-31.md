# Mixed delivery, travel and boss Retry regression ready offline

Stage: workspace `build/ue4ss-receipt-acceptance-r7-20261101`. Prepared only; nothing installed, armed or watching. Passed dispatch evidence remains in `ue4ss-owner-dispatch-passed-2026-10-31.md`.

## Scope and recoverability

The passed C owner/transport DLLs are reused byte-for-byte. Source in `experiments/ue4ss-dispatch-regression` is alongside the passed dispatcher. Only routine SERVICE logging changes: first handoff, status/invariant changes and periodic summaries replace one line per request. A source-reconstruction audit recovers the passed pump text after undoing those diagnostic edits; host.cpp differs only in its version label. Dispatch frequency, actual calls, validation and authority logic are unchanged. This is a behavior-preserving diagnostic refactor, not a new gameplay implementation.

Verified recovery archive: `backups/ue4ss-r6-passed-before-regression-20261101`, including source/binaries/config/manifests/tools, hashes and RESTORE.txt. Original stable runtime/prototype and development/playthrough profiles remain unchanged. The experimental stage reuses only the existing private UE4SSReceipt20261031/APReceiptFixture profile under the r1 profile root.

The next session extends live coverage of the already passed dispatcher to representative nonconsumable lanes, world transitions/restart, combat and boss Retry. It does not claim that all those engine calls have been converted to reflection: their existing native implementations remain in the C owner.

## Exact fixture history and equipment

The twelve old identities are extracted from the closed r6 full index-zero network snapshot and cross-checked against last_admitted_history. The fixture no longer reconstructs old identities by positions in a changing location catalog. A sealed fixture-history.json preserves exact locations/items/order. Unknown/partial/conflicting histories are refused; nothing is inferred empty.

Explicit `send` releases thirteen items in one batch:

- Cartwheel, Barrier, Cure Magic, Spellweaver.
- Two Progressive Thunder items: Thundaga and Thundaja.
- Doubleflight, Air Slide and Shotlock.
- One Max HP Increase and one Max MP Increase.
- The World Within Access and Arm Guards.

The batch exercises different existing delivery/projection paths without testing every wardrobe piece or collecting checks across worlds. Server order is authoritative and need not match the list above. Wait for all thirteen acknowledgements/popups before menu/travel checks. Record HP/MP maxima before the batch; expect the normal +5/+10 gains. Do not mistake normal damage/spell expenditure/save-point healing for a projection failure.

An explicit later `send2` releases one Hi-Potion at index 25 after world change/Retry to test the UE4SS consumable bridge against the new actor/context. Its location and all kit locations are synthetic Castle chest/gear checks, outside the Forest/WW route. No Castle access is supplied, so route activity cannot release these rewards accidentally. Normal route checks absent from the private multidata are ignored by the real AP server, as its register_location_checks implementation specifies.

The private fixture enables existing DeathLink using slot schema6/item schema6 with zero trap percentage. This permits one controlled defeat for boss Retry, avoiding a long wait for the boss to kill Aqua. No new native death writer is implemented. The schema normalizes to the exact original profile-binding fields; normal open_profile reused a copied profile without changing its identity. The real private profile was not edited. A real local Bounced-message test reaches the normal DeathLink receiver with no native owner attached. It is network-fixture proof, not live DeathLink proof under the SDK dispatcher.

`defeat` waits for normal cached DeathLink eligibility, then sends one event through the authenticated local server. It is latched before awaiting the network send and never retries an uncertain native effect. It must be requested only after the user reports control in the actual Phantom fight. Queued death is applied only when the unchanged DeathLink service permits it; unpause to allow application. The original remote-death echo suppression remains in charge.

## Offline acceptance

Native host/import/export and diagnostic-gate tests pass, including first/status/bootstrap/counter-anomaly reporting, periodic summaries and no idle spam. Existing native weak/lifetime/receipt tests pass. The unchanged receipt adapter/core passes 67 tests; four Lua context tests pass. Five readiness tests, three control/cleanup tests and six strict history-extraction tests pass. The control tests cover waiting for eligibility, one death event only and retaining the runtime lease through failures/cancellation.

Real loopback AP tests validate the exact twelve-item prefix, the thirteen-item mixed batch, the late Hi-Potion, duplicate checks/Sync, out-of-order refusal and the bounded DeathLink message path. Original copy-only resume_records/read_associations admits slot1 against the twelve old receipts. Copied decode shows slot2 empty (zero level/flags and empty map), and a normal profile-open test verifies the new transient DeathLink configuration still uses the same existing profile identity.

Final hashes freeze current tools, the source/config/binary archive, fixture history, private save/journals and copied evidence. Explicit live mode without user readiness refuses. All networking used for offline tests was closed; no game was launched.

## One combined live session, only when user returns ready

1. Keep the game and ordinary AP client closed. Verify final hashes, take a fresh verified recovery backup and install r7 with the existing installer. Start `py -3.13 tools/run_ue4ss_dispatch_regression_20261101.py live --user-ready --stage build/ue4ss-receipt-acceptance-r7-20261101`. User launches manually and waits at title, then loads existing private slot1 after attachment.
2. Require owner active, fresh/weak-ready context, approved core and SDK handoff. Freeze this run's bootstrap counter; do not assume it equals r6's 63. Confirm no old item popup. Record baseline resource maxima, use `send --stage ...`, then wait for all thirteen acknowledged items. Check movement/defence, available Thunder/Cure versus still-locked Fire/Blizzard, Shotlock, the stat gains, wardrobe and the new world entry. Remap any consumable shortcut to Cure before combat to avoid accidental inventory changes.
3. Save private slot1 with the kit. Travel to WW through normal Teleportation, skipping opening scenes if desired, and report control at its save point. Inspect fresh context/SDK activity, resource maxima and retained kit. Perform one normal Restart WW, return to that save point and inspect the same invariants. No full world clear is needed.
4. Save a separate private checkpoint in slot2 if it is visibly empty as expected. This preserves the Forest checkpoint in slot1 and makes future WW testing quicker. Do not overwrite an unexpected occupied slot. This is an ordinary game save, not file copying/adoption.
5. Reach the first Phantom Aqua fight by a short known room route; the Mines are unnecessary. All movement items are supplied. No Zodiacs/Wayfinders are given, so the boss-rush mirror should not interrupt the story route. Pause with control before attacking. Check combat movement/defence/magic as convenient; no win is required.
6. Use `defeat --stage ...` only when the user is ready in that boss fight. Ask them to unpause; require one normal defeat screen and one received DeathLink attempt, with no echo/repeated kill. User chooses actual Retry, then pauses after regaining control. Inspect owner/epoch, current references, SDK counters and retained kit after recovery. Do not substitute a normal-enemy Continue for this boss-Retry criterion; they are distinct proven systems.
7. Use `send2 --stage ...` for one Hi-Potion after Retry. Unpause for its popup and verify one added Hi-Potion (normally 1 to2, unless the user consumed one). Require exact-inventory acknowledgement through the UE4SS bridge on the new context. Request Sync once; no duplicate. The user can close normally from the fight; no need to beat Phantom just to finish testing.
8. Wait for coordinator cleanup/controller exit. Preserve full log, private profile/journals and development files. Verify all thirteen kit acknowledgements plus the late item if released, the DeathLink/recovery events, owned-record provenance, SDK activity and unchanged bootstrap count. Decode the copied save. Slot2 was saved before the late item, so a missing late Hi-Potion in that manual checkpoint is expected; its AP identity must remain in the authoritative history for normal replay on a future load. Do not call that a failed persistence test or patch inventory to compensate.
9. Remove only unchanged owned additions and restore the exact prior stable runtime/metadata under RuntimeLease. Source profile is under r1, not r7/profiles. Preserve unknown changes and every save. Full history is 25 after the kit, or 26 after the late item; use actual closed history if stopping early. Never reconnect an older truncated fixture.

If a route, recovery or delivery refuses, preserve its original pending operation and evidence; never replay an uncertain effect or silently weaken guards. A failed stage should not turn into repeated blind launches. Early bootstrap removal, permanent/explicit end-user installation and broader framework memory lifetime remain separate work.

Never launch/close automatically, broad-scan process memory, adopt/delete/restore saves or run an external writer beside the shared owner. Nothing has been pushed or released.
