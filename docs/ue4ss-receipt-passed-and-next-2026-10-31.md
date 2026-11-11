# UE4SS receipt milestone passed; reconnect/queue candidate ready offline

Update: the r3 reconnect/queue test passed. Its closed evidence and current unarmed r4 hitch candidate are documented in `ue4ss-hitch-investigation-ready-2026-10-31.md`. The r3 readiness instructions below are historical; do not reconnect its now-seven-item profile with a one-item history.

This supersedes the readiness status in `ue4ss-receipt-bridge-r2-ready-2026-10-31.md`. It is not a public-package migration claim.

## Proven in the closed r2 test

Workspace capture: `research/ue4ss-receipt-live-20261101T001410411937Z`. The timestamp is UTC; the local session was October 31.

The normal packaged AP coordinator attached and the shared owner became active. UE4SS validated the current controller/pawn with the SDK Object root filter, independent controller/pawn getters and original thread/owner checks. Both item function contracts passed, including the distinction between reflected grant ParmsSize 17 and aligned frame size 24.

One real local AP history entry, Hi-Potion at index 0, went through the experimental UE4SS converter and grant. There was exactly one native grant entry and return. The original owner recorded inventory 0 to 1, entered=1, status=1 and an acknowledged receipt. Repeated AP Sync did not produce a duplicate. The user saved private slot 1, returned to title and reloaded: one Hi-Potion remained and no popup repeated. The new context was accepted after reload. The game closed normally.

The complete private profile, journals, source/binary/config archive and UE4SS log are preserved. Normal coordinator cleanup completed; eleven unchanged experimental additions were removed; existing XInput remained. The exact original stable runtime DLL and runtime.json were restored under its exclusive RuntimeLease. The original prototype PAK was unchanged. The development KH0.2BbS_WW.png is byte-identical to its pre-test copy, which also proves every development record unchanged. KH2.8_system_WW.png changed and was copied/preserved, not restored; its internal reason has not been established. No save was adopted, deleted or restored.

This establishes one consumable receipt through UE4SS, including same-process title/reload. It does not yet establish cross-process reconnect, every consumable mapping, long-session observation lifetime, all-world delivery, replacement of the owner dispatcher, or replacement of early save isolation.

## Hitch investigation and alongside candidate

The user reported a brief hitch at delivery. The log contains 3,431 lines, including 424 object-validity traces, 54 fresh-instance lookup traces, 160 stable lookup traces and 106 function-contract traces. Conversion took 281 ms and grant 235 ms; these timings do not include all external permission checks. Logging may contribute, but the lookup cost was not separately measured. Do not claim the cause is fixed.

The passed `experiments/ue4ss-receipt-host` remains unchanged. New `experiments/ue4ss-receipt-host-quiet` compiles out seventeen successful diagnostic trace sites and adds a compact timing line around permission checks. Refusal/exception messages, grant contract readiness, grant entry/return and operation timings remain. Contract mismatches emit their actual metadata on refusal. The fresh lookups, pointer-identity comparison before reads, SDK validity checks, concrete classes, independent getters, thread/owner checks, frame checks, receipt membership and inventory acknowledgement remain intact. No pointer cache or gameplay-state mutation was introduced. This is a behavior-preserving diagnostic change; its performance benefit is unproven until live.

Recovery archive: `backups/ue4ss-receipt-r2-passed-before-quiet-20261101`. Its verified hashes and RESTORE.txt preserve the passed source, binary, full stage, config and original tools. The stable SQLite-portable APWorld and normal installed prototype remain the baseline.

## Next test: one session, existing private slot 1

Stage: `build/ue4ss-receipt-acceptance-r3-20261101`. Nothing is installed or armed. The stage intentionally reuses only the private UE4SSReceipt20261031/APReceiptFixture profile beneath `build/ue4ss-receipt-acceptance-20261031/profiles`. It never uses the user's real playthrough or vanilla slot 1.

The new controller is `tools/run_ue4ss_receipt_resume_20261101.py`. It reconstructs the exact prior Hi-Potion history identity before client connection, then appends six distinct new receipts only on explicit send: Potion, Ether, Mega-Potion, Mega-Ether, Elixir and Megalixir. It does not reset to empty history. Synthetic location IDs are selected from the actual catalog, not assumed contiguous. The room is a private loopback fixture and never connects to a public server.

The existing decoder validated the copied post-test save: slot 1 has Hi-Potion 1 and Ether/Potion 0. The copy-only resume preflight uses original read_associations/resume_records and the replayed history; exact association passes without adopting a record. The current private save and journals are pinned by the stage's final hashes before another installation. No offline preparation modifies them.

Offline acceptance includes 67 C receipt/adapter regressions, native dispatcher tests for all seven consumable IDs and invalid/revoked/reentrant/uncertain cases, import/export checks, cleanup-lifetime regressions, real AP loopback admission with the prior item, six-item append-only history, repeated check and Sync, exact copied-record resume, source/binary/config archive verification and explicit-readiness refusal. This is not evidence of live queue success.

Only when the user returns ready:

1. Verify final hashes and the existing private slot-1 association; take a fresh verified installation recovery backup. Install with `py -3.13 tools/install_ue4ss_read_probe_20261031.py build/ue4ss-receipt-acceptance-r3-20261101`.
2. Start `py -3.13 tools/run_ue4ss_receipt_resume_20261101.py live --user-ready --stage build/ue4ss-receipt-acceptance-r3-20261101`. User launches manually with the ordinary AP client closed. Verify title attachment before loading private slot 1.
3. Require accepted current context, owner active and grant contract ready. Confirm one Hi-Potion and no repeat popup before sending anything. No combat, Main Road or new game is required.
4. Use the same script with `send --stage ...`. Allow all six queued popups to finish and confirm one of each new consumable, with Hi-Potion still one. Record whether the hitch improved; compare native operation/check timings rather than assuming logging was the only cost.
5. Request `sync --stage ...`; require no duplicates. Save private slot 1, title/reload, and require the seven counts remain one with no repeat popup/grant. User closes normally.
6. Wait for normal coordinator cleanup and controller exit before removing only unchanged owned additions. Archive the log and entire private profile, verify receipt entries/inventory and development saves, then restore the original stable runtime/metadata under RuntimeLease. The reused profile is under the r1 profile root, not r3/profiles. Never overwrite an unknown change.

The previous r2 controller/manifest are preserved as historical evidence, but must not be used as an empty-history reconnection to the now nonempty profile. r3 final hashes deliberately become stale when the next live save/journals change; record the closed result rather than silently re-freezing a used test as ready.

## Subsequent integration order and boundaries

1. Finish the narrow receipt reconnect/queue milestone. Keep all nonconsumable delivery and owner policy unchanged.
2. Address event-context lifetime and dispatch before expanding gameplay replacements. The experimental 32-observation cap must not ship as a production session policy.
3. Substitute reflected reads/calls one subsystem at a time where exact functions and lifecycle are established. Stats/reapplication, wardrobe/zodiacs, notifications and gates have different contracts; a generic grant is not proof they can be merged.
4. Implement explicit player installation, update/removal ownership and an explained inactive mode. Preserve early isolation until a replacement proves equivalent before save-data initialization. UE4SS initialization has not been proved early enough.
5. Run focused regressions for changed paths, then an integrated playthrough. Existing proven systems are not to be rebuilt merely because UE4SS is available.

Pinned SDK source audit: UE4SS/src/Mod/LuaMod.cpp:3055 registers ExecuteInGameThread, queues an action and registers a ProcessEvent pre-callback. The drain at 2917 sets a Lua game-thread marker without independently checking the OS thread; its source explicitly notes the asynchronous-UFunction assumption. This is a standard scheduling facility worth evaluating, not permission to discard our actual thread check or claim a reliable owner scheduler. CppMod.cpp:139 forwards on_update without establishing a gameplay-thread guarantee. No new tick/polling/ProcessEvent hook or framework modification is installed in r3.

All source changes remain local. No release, push, game launch/close, broad process-memory scan, save adoption/deletion or parallel external writer is authorized by this preparation.
