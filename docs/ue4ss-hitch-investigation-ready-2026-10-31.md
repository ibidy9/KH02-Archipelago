# Receipt hitches: evidence and weak-reference candidate

This supersedes the current-test status in `ue4ss-receipt-passed-and-next-2026-10-31.md`. All changes are local; the working public runtime is preserved.

## Closed r3 result

Capture: workspace `research/ue4ss-receipt-live-20261101T004024884687Z`. The original Hi-Potion survived a complete process/controller restart with no popup or duplicate. All six other consumables arrived through UE4SS, each produced an acknowledged receipt with inventory 0 to 1, and the seven counts remained one after Sync and save/title/reload. There were exactly six native grant entries. The user saw smooth queued popup animations after the delivery hitches, which supports separating notification display from receipt execution cost.

Normal coordinator cleanup completed, eleven unchanged owned additions were removed and the exact original stable DLL/runtime.json were restored through RuntimeLease. The installed original prototype was unchanged. The development KH0.2BbS_WW.png is byte-identical to its pre-test copy, so all its records are unchanged. The collection KH2.8_system_WW.png changed, was preserved/copied, and was not restored. No save was adopted, deleted or restored. The whole private profile, journals and archive are preserved. Decoding its copy and exact-association preflight confirmed nine associations, two admitted distinct records and the seven-item history.

## Hitch evidence

The quieter host still produced repeated hitches. The six-item capture contains 96 successful exported permission checks: minimum 46 ms, median 47 ms, maximum 78 ms, total 4,937 ms. The twelve converter/grant operations total another 2,845 ms. These are separate exported calls; internal permission resolutions are already included in operation timings and must not be counted twice. This totals about 7.8 seconds of sampled synchronous work across the six receipts, not a measured continuous freeze duration.

Source inspection identifies a strong candidate: every backend resolve repeats two current-instance exact-name lookups. Converter/grant paths repeat resolves, plus a pawn receiver lookup. The SDK StaticFindObject template forwards to FindObject. With a non-null Object class, FindObject bypasses its name-only global cache. The Object root has no snapshot search pool; its search iterates the object registry, testing class/package/name. The SDK then caches the result, but a subsequent non-null-class call still bypasses that cache. Returning to the untyped cache is unsafe: earlier load/reload tests demonstrated a stale controller address there.

The timings measure whole checks and operations, not isolated name-search calls. Thus repeated registry searches are a source-supported explanation, not a separately timed live proof of the sole cause. Quieter logging failed to solve the problem. No broad process-memory scan or new object-registry traversal was performed during investigation; analysis used pinned SDK files and closed logs.

## Candidate: standard SDK weak references

Passed sources and binaries remain unchanged in `experiments/ue4ss-receipt-host` and `experiments/ue4ss-receipt-host-quiet`. New `experiments/ue4ss-receipt-host-weak` is developed alongside them. Recovery archive `backups/ue4ss-receipt-r3-passed-before-weak-20261101` contains verified source, binaries, config, manifests, original tools and RESTORE.txt. Pre-test installed prototype/runtime backups remain available through the recorded r3 installation backup.

At a possession event, the candidate still performs the passed full fresh-name lookup, identity-before-read check, concrete class checks and independent controller/pawn getter checks. Only then does it capture two SDK FWeakObjectPtr values. They contain indices/serials, not strong references or retained UObject pointers. No serial allocation/write or GC rooting is added.

The pinned SDK assignment operator reads the verified object's index and existing engine serial. Get rejects a zero serial, negative/missing slot, changed serial and unreachable/pending-kill item before returning the current slot object. The 4.13+ container implementation used for this engine bounds-checks the index. The candidate compares the returned address with the independently captured event identity before any object reads, then retains the original live validity, concrete type, independent getter, thread, owner-permission, revision, frame, membership and inventory checks.

When both handles work at capture, repeated current-instance searches use Get. A subsequently invalid or mismatched ready handle fails closed; there is no fallback that could accidentally accept a replacement object at a reused address. When an engine serial is unavailable at initial capture, the candidate reports this and retains the exact previous name-search path. It does not invent a serial or silently weaken acceptance. WEAK_CONTEXT reports readiness separately for controller/pawn.

This is a compatibility/performance improvement requiring live validation, not merely a logging refactor. The actual weak layout and serial availability for these objects are not yet live-proven. The expected performance benefit is unproven. If the initial handles are unavailable, inspect the captured diagnostic and stop rather than asking the user to repeat slow queues.

No owner/receipt policies, original C ABI, reflected dispatcher, transport adapter, item mapping or Lua hook changed. No new scheduling hook, background writer, timer, SDK modification, unsafe pointer cache or silent legacy grant fallback is introduced. The 32-observation experimental limit remains; this is not a production lifetime policy.

## Offline verification

Stage: `build/ue4ss-receipt-acceptance-r4-20261101`. Its final-verification.json freezes sources, tools, binaries, config, copied evidence and the current private save/journals. Nothing is installed, armed or watching.

The native build/import/export audit passes, including the required SDK FWeakObjectPtr exports. Dispatcher regressions still cover all seven consumable IDs and invalid/revoked/reentrant/uncertain effects. Added identity tests cover a valid reference, same address with changed serial, zero serial, dead object, wrong address, zero expected identity and validator refusal; invalid identities never reach object validation. These use synthetic references and do not substitute for live SDK tests. The unchanged C adapter/receipt core passes 67 regressions. Cleanup-lifetime/readiness tests and the explicit-user-readiness guard are retained.

The fixture reconstructs the seven exact prior identities from r3, then appends Potion at index 7 and Ether at index 8 on separate controls. Real local AP loopback tests verify that initial history exactly matches the closed capture, prefixes remain unchanged, repeated checks/Sync do not duplicate, and releasing the second new item before the first is refused. Copy-only resume_records/read_associations verifies the existing private slot 1 against all seven previous items. No profile or save is edited offline.

## Next live test, only after the user returns ready

1. Verify final hashes and take a fresh recovery backup. Install with `py -3.13 tools/install_ue4ss_read_probe_20261031.py build/ue4ss-receipt-acceptance-r4-20261101` while game/ordinary client are closed.
2. Start `py -3.13 tools/run_ue4ss_receipt_weak_20261101.py live --user-ready --stage build/ue4ss-receipt-acceptance-r4-20261101`. User launches manually; inspect title attachment before loading existing private slot 1. The profile root remains under r1, not r4/profiles.
3. Require owner active, accepted current context, GRANT_CONTRACT ready and WEAK_CONTEXT ready=true for both objects. Confirm no replay popup and the seven old counts remain one. If weak readiness fails, do not send benchmark items; preserve evidence and finish cleanup normally.
4. Use `send --stage ...` on the r4 script for one Potion. Require a normal popup, count 2, one native entry and acknowledged receipt. Compare permission and operation times with the r3 baseline and ask about the hitch. Request Sync and require no repeat.
5. Save private slot 1, title/reload. Require Potion 2, other counts 1, no repeat, and fresh accepted/weak-ready context. Use `send2 --stage ...` for one Ether. Require Ether 2 and a second acknowledged native grant without a hitch. This exercises a newly loaded object generation rather than only the first load.
6. Save slot 1; user closes normally. Inspect native and durable evidence, wait for coordinator cleanup/controller exit, copy the full private profile and development profile, remove only unchanged manifest-owned additions and restore the exact previous stable runtime/metadata under RuntimeLease. Preserve unknown changes and every save. The next fixture must reconstruct nine acknowledged identities if both new deliveries pass; never reconnect with an older truncated history.

No new game, combat, chest, world traversal or full queue replay is required. No game launch/close is automated. If the weak path fails, revert promptly to the preserved passed host; it remains functionally correct, and the stable public runtime has not been replaced.
