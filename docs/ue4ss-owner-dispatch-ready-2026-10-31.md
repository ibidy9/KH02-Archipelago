# UE4SS owner dispatch candidate: offline ready

Stage: workspace `build/ue4ss-receipt-acceptance-r6-20261101`. Nothing is installed, armed or watching. Current source is alongside the passed versions in `experiments/ue4ss-owner-dispatch`. The previous lifecycle result remains in `ue4ss-context-lifecycle-passed-2026-10-31.md`.

## Concrete change

Earlier tests changed current-object lookup, native calls and context lifetime; the original Windows message callback still dispatched owner requests. This candidate changes that dispatch path after startup. It is not another read-only scheduling probe.

The original startup storage/attachment remains. After an independently validated Aqua context, one native UE4SS ProcessEvent post-callback binds the already initialized C owner and services its existing shared-memory requests. The window hook then stops servicing every command. Later Windows wakes are ignored, not used as a gameplay fallback. Before handoff, receipt command 5 is also barred from the window path; bootstrap/control/cleanup can still work. Thus a failed SDK handoff cannot accidentally make a receipt test pass through the old mechanism.

The callback executes only on the independently identified actual owner/window thread, with reentry exclusion and a short bounded polling interval. Other ProcessEvent threads return before touching owner-thread state. The callback allocates no Lua task per frame or request. The previous three scheduling probes were retired from this candidate; Lua retains only the exact ClientRestart context invalidation/observation hook. The SDK already owns the ProcessEvent detour; this registers one native post-callback on it.

The same C fresh_service body executes commands and retains its exact cookie, authority, process/thread, sequence, phase, owner, receipt membership, inventory and acknowledgement rules. No second owner, parallel writer, alternative receipt queue or new save format is introduced. C transport mapping fields are unchanged. A versioned 48-byte status API reports dispatch source/counters. The only new core export is Kh02SdkOwnerPump.

This is an experimental compatibility/architecture change needing live evidence. The post-callback's idle/paused liveness and suitability across game phases are not established by the previous read-only helper tests. Original phase guards stay in force. Do not call it a complete replacement of the custom loader, early save isolation or game hooks: those remain unchanged.

## Module identity and lifetime

The host queries only the already-loaded core at LOCALAPPDATA/KH02AP/runtime/Kh02FreshTransport.dll, the normal client's stable runtime path. It normalizes/checks the exact module path, verifies the file against the two approved experimental fresh/resume transport hashes, pins the module and validates the versioned status ABI/process/thread. It never loads a core itself or resolves a bare-name DLL. The normal client still controls its exclusive runtime lease, integrity checks and original-process cleanup.

A missing/unapproved module or failed pump stops/defer work. There is no post-handoff switch back to Windows dispatch. If the SDK cannot drain a pending request, never replay an uncertain effect or release its code: preserve evidence, stop delivery and wait for normal game exit as the existing coordinator already requires. Native std::exception diagnostics identify sdk_owner_dispatch; access violations are not swallowed. Callback/module code remains process-lifetime pinned and hot reload is disabled.

The approved core hash header is generated from the actual two newly built transports. Rebuilding those requires regenerating the header, rebuilding the host and re-freezing the stage. This pins our package components, not a particular system SQLite/Windows DLL version.

## Preserved implementation and evidence

Verified recovery archive: `backups/ue4ss-r5-passed-before-dispatch-20261101`, including source, binaries, config, manifests, tools, hashes and RESTORE.txt. Original stable package/prototype/runtime recovery copies remain recorded by prior install backups. Passed weak/lifecycle sources were not overwritten.

`build/ue4ss-owner-dispatch-core-20261101` copies the previous Forest transport sources. Each variant differs only in the controlled transport wrapper and three added pump headers/include; the original owner/service body and all remaining source files match the passed transport overlay. Both fresh/resume variants compile, but this next live test uses resume only. Original production common/variant sources and binaries remain unchanged.

The private profile has TEN acknowledged identities. Its copied manual slot 1 has Potion 3, Ether 2 and Hi-Potion 1. Original resume_records/read_associations admits that exact snapshot against the reconstructed history. Old fixtures with fewer identities must not reconnect it. No save/profile was edited offline.

## Offline acceptance

- Native pump fixture tests ABI, absent/uninitialized state, actual-thread/setup/reentry guards, closed-before-bind refusal, bootstrap/receipt boundary, permanent handoff, duplicate sequence suppression and cleanup servicing after bind. This uses a mocked original service, not a live engine.
- Shared loader code runs in standalone Windows fixtures with a Unicode profile path: absent core stays unloaded; a same-name decoy is not called; an unapproved hash is not called; the exact already-loaded approved mock is called once. No game, UE4SS or real transport DLL is loaded into these tools.
- Native host build/import/export audits and existing weak/lifecycle/receipt dispatcher fixtures pass. The C core adds exactly one export and no test-only controls. The 67 C adapter/receipt regressions and four Lua lifecycle tests pass. Five readiness and two cleanup-lifetime tests pass.
- Real private loopback AP tests reconstruct all ten identities exactly, append Potion/Ether at indices 10/11, preserve prefixes, reject out-of-order second release and duplicate checks/Sync, and close all networking. Copied-record resume passes without adoption.
- final-verification.json freezes sources, complete copied core build, tools, binaries/config, fixture results and current private profile/journals. Explicit live mode without user readiness refuses before preparation.

These checks cannot prove real frame-phase safety or paused delivery. That is the purpose of the next live session.

## One batched live session, only when user returns ready

1. Keep game/ordinary client closed. Verify final hashes and take a fresh recovery backup. Install `build/ue4ss-receipt-acceptance-r6-20261101` with the existing installer.
2. Run `py -3.13 tools/run_ue4ss_owner_dispatch_20261101.py live --user-ready --stage build/ue4ss-receipt-acceptance-r6-20261101`. User launches manually, waits at title; verify original attachment. Title still uses the proven bootstrap.
3. User loads existing private slot 1 and stands with control. Require active owner, new accepted/weak-ready context, CORE_VERIFIED and SERVICE bound=true. No replay popup. Record the bootstrap command count at handoff; it must stay fixed afterward while sdk_commands/sequence advance. ignored_wakes should demonstrate Windows wake messages no longer service requests. Do not send if handoff is missing or refused.
4. Send one Potion with `send --stage ...` while standing. Require a smooth popup/count 4, one SDK-serviced owner request and one acknowledged receipt at index 10. This is the idle case.
5. Ask user to pause. Send Ether using `send2 --stage ...`, inspect whether the SDK can service the paused request, then unpause for its normal popup. Require Ether 3 and one acknowledged receipt at index 11, with no legacy dispatch. If paused requests do not drain, do not replay the effect: inspect the pending original sequence and retain ownership through normal cleanup.
6. Request Sync; require no duplicates. Save private slot 1, title/reload and require Potion 4/Ether 3/other original counts, no repeat popup/grant, fresh context and continued SDK servicing with unchanged bootstrap count. This also checks continued dispatch when returning through title with no valid Aqua hint.
7. User closes normally. Wait for original coordinator cleanup and controller exit. Archive full logs/private profile/development files; verify exactly two new acknowledgements and the saved counts. Remove only unchanged owned additions and restore the exact prior stable runtime/metadata under RuntimeLease. The reused profile remains under the r1 profile root, not r6/profiles. Successful history will then have twelve identities.

No new game, combat, chest or traversal is required for this dispatch boundary. Further combat/travel/retry regressions should be batched once this replacement actually works; do not claim the whole integration passed from idle/pause alone. Fresh startup/bootstrap removal and explicit end-user installation remain separate work.

Never launch/close the game automatically, broad-scan memory, adopt/delete/restore saves, or run an external writer beside the shared owner. Nothing has been pushed or released.
