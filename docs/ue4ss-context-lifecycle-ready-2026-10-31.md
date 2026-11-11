# Context lifetime candidate and read-only scheduling check

Current stage: workspace `build/ue4ss-receipt-acceptance-r5-20261101`. Prepared offline; not installed, armed or watching. The passed weak-reference delivery milestone remains documented in `ue4ss-weak-receipt-passed-2026-10-31.md`.

## Preserved baseline

The passed host is unchanged in `experiments/ue4ss-receipt-host-weak`. Its two deliveries before/after reload were smooth, acknowledged, and persisted. The private test profile now has nine authenticated receipt identities. Normal installed startup/runtime/prototype and development saves remain untouched by offline preparation.

Before editing, source, binaries, stage/config/manifests and original tools were archived and hash-verified in `backups/ue4ss-receipt-r4-passed-before-lifecycle-20261101`, with RESTORE.txt. New source is alongside it in `experiments/ue4ss-receipt-host-lifecycle`. No proven implementation was overwritten.

## What changes

The old prototype stopped both native/Lua observation after 32 possession events. Simply lifting that bound would leave lifecycle questions unanswered. The candidate replaces this research cutoff with one bounded current context, 4096-byte input-name limits, reentry exclusion and checked generation increments. It does not accumulate an event history, object list or per-event timer.

The existing exact ClientRestart hook now clears the context in its pre-callback. Post-callback facts must pass the same fresh lookup, identity-before-read, concrete class, independent getter and actual thread checks before publishing new SDK weak references. A title/non-Aqua refusal leaves the hint unavailable rather than leaving the previous accepted hint as current. Generation exhaustion clears the context and refuses publication instead of wrapping. Existing request generation checks refuse work if a callback changes the context during a receipt operation.

The C receipt ABI, adapter, dispatcher and weak-identity helper are byte-identical to the passed candidate. Original owner, storage, bounded reads, membership, inventory verification and acknowledgement remain unchanged. This is a lifecycle compatibility improvement requiring live validation, not just a behavior-preserving formatting refactor. No claim is made about every travel/retry lifecycle from a load/reload test alone.

## Standard scheduling helper: observation only

The script queues at most three ExecuteInGameThread callbacks: startup and the first two accepted contexts. Each invokes a native read-only probe that checks the actual own-window/OS thread and copies only plain context metadata. It does not resolve UObjects, invoke owner permissions, grant items, touch saves or acknowledge receipts. Wrong-thread execution is reported as refused. No recurring callback, worker writer, tick hook or new owner dispatcher is added.

Pinned source audit found:

- LuaMod.cpp:3055 queues work; its drain around 2917 sets a Lua game-thread marker without independently proving the OS thread. The source notes its assumption about asynchronous UFunctions.
- UnrealInitializer.cpp:499 already installs the framework's ProcessEvent detour. ExecuteInGameThread adds a callback to that existing hook; this candidate does not introduce a separate custom detour.
- make_hook_state creates/records a Lua thread for each queued action. LuaMadeSimple::new_thread stores wrapper bookkeeping in a process map. Completed action registry references are released, but these other containers are not shown being pruned per action. This is a source-supported long-session growth concern, not a measured leak. Therefore this helper must not become a per-tick/per-item production pump without addressing that lifetime.
- CppMod::fire_update is reached from the framework update loop; its name is not proof of gameplay-thread execution.

The three-probe limit bounds this experiment's extra scheduling state. The mod's bounded-context claim does not establish bounds on all SDK name caches or logs. No framework cache is cleared, no SDK source is patched and no borrowed framework implementation is rebuilt.

If scheduling is reliable, evaluate a single bounded native queue with an independently checked callback, retaining owner authority. Do not replace the owner dispatcher on the strength of this read-only test. If the helper runs on an unsuitable thread or fails to drain, retain the proven owner and inspect the captured result rather than adding arbitrary delays/retries.

## Offline evidence

Native build/import/export checks pass. Native fixtures cover 10,000 generations, overflow refusal, reentry exclusion, exception release, original weak-identity failures and receipt dispatcher cases. Four Lua 5.4 tests use fake framework callbacks: 80 events without a cutoff, invalidation before facts, title refusal followed by a fresh observation, at most three queued probes and wrong-thread/unavailable scheduler reporting. They load neither UE4SS nor the game and do not prove live framework behavior.

The 67 C receipt regressions, five startup-readiness tests and two cleanup-lifetime tests pass. Real local AP loopback tests reconstruct the exact nine previous identities from the closed r4 capture, append one Potion at index 9, preserve history prefixes and reject duplicates from repeated check/Sync. Copy-only original resume_records/read_associations admits slot 1 with 13 associations/two distinct records. No save was adopted or modified. Explicit live mode without user readiness refuses before arming. final-verification.json pins current sources/tools/binaries/config/profile/journals.

## Next live test, only when the user returns ready

1. With the game and ordinary client closed, verify final hashes, take a fresh recovery backup and install `build/ue4ss-receipt-acceptance-r5-20261101` using the existing installer.
2. Run `py -3.13 tools/run_ue4ss_receipt_lifecycle_20261101.py live --user-ready --stage build/ue4ss-receipt-acceptance-r5-20261101`. User launches manually and waits at title. Inspect attachment and startup scheduling result. Native scheduling logs are SCHEDULE_PROBE; the Lua wrapper logs SCHEDULE_RESULT. Neither is AP authority.
3. User loads existing private slot 1. Require active owner, pre-callback CONTEXT_INVALIDATE, post-callback CONTEXT_ACCEPT with a newer nonzero generation, accepted context, weak references ready and grant contract ready. No old item popup should occur; Potion/Ether are initially 2, other consumables 1. Inspect the loaded-context scheduling result independently of receipt authorization.
4. Send one Potion with `send --stage ...`. Require count 3, one normal popup, one native entry and one acknowledged receipt at index 9, with smooth delivery. Request `sync --stage ...`; require no repeat.
5. Save private slot 1, title/reload. Require the old hint was invalidated, a newer context accepted, weak references ready, Potion 3/Ether 2 and no repeated popup/grant. Collect the third bounded scheduling result. No new game, combat, chest or world traversal is needed.
6. User closes normally. Wait for coordinator cleanup/controller exit. Archive logs/full private profile/development files, verify receipt and decoded saved counts, remove only unchanged owned additions, and restore the exact pre-test stable runtime/metadata under RuntimeLease. The reused profile remains under the r1 profile root, not r5/profiles. Do not treat a missing scheduling result as permission to replay an effect.

If delivery succeeds, future fixtures must preserve ten history identities. Preserve all unknown changes and development/playthrough saves. Never launch/close automatically, broad-scan memory, adopt/delete/restore saves, or run an external writer alongside the shared owner. Nothing has been pushed or released.
