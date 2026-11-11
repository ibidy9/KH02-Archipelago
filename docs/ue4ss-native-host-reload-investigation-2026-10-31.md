# Native host: first load passed, reload failed

**Follow-up:** The checkpoint probe reproduced the reload failure after the untyped lookup returned the first load's controller address. It was removed. Read `ue4ss-native-host-fresh-lookup-ready-2026-10-31.md` for the narrowed evidence and unarmed correction candidate.

This supersedes the test status in `ue4ss-native-host-ready-2026-10-31.md`. The native host is experimental and must not be integrated into the working AP package yet.

## Evidence

Workspace capture: `research/ue4ss-native-host-live-20261031`. The original log, failed source/binary/config archive, installation manifest, closed profile copy, decoded recovery copies, per-record comparison, result and SHA256 manifest are preserved there. The pre-test recovery backup is `backups/before-ue4ss-native-host-probe-20261031-20261031T202648Z`.

On the first load of development slot 62, the native callback and the process's window thread both reported 6460. Independent native controller/pawn getters matched the Lua context. Read-only Potion and Hi-Potion conversions returned nonzero names. No item was awarded and no AP authority was established.

Returning to title produced the expected non-Aqua refusal. Reloading reached the second native `THREAD ... same=true` entry at 20:31:34 in the log, then the user reported a fatal error. There is no subsequent native CONTRACT, NATIVE_CONTEXT, CONVERT or Lua accepted result. This is a **failed reload**, not a successful native-host milestone. The last flushed message brackets subsequent work; it is not an instruction address or a crash stack.

The game was closed by the user. All eleven unchanged, manifest-owned additions were removed and the existing identical XInput DLL was preserved. No save was edited or restored. Both copied save containers passed the decoder's integrity checks. Only autosave record 0 changed; all manual records, including 62 and 64, are byte-identical to the actual fresh pre-test backup.

## Offline findings and limits

The next native operations were conversion of copied full names, named controller/pawn lookup, object-slot validation and class checks, before getter-contract logging. No new crash dump/report was found in the checked game directory and standard crash-report locations. The latest matching Windows archived report was from August 30 and was excluded as unrelated.

The pinned SDK's StaticFindObject routes through FindObject, which can return a cached UObject before searching. ClassListener.cpp has deletion invalidation, so a stale cache entry is a hypothesis rather than a demonstrated defect. The host's live() check reads the returned object's index before checking its array slot; this does not make an already freed pointer safe. The prior Lua getter calls succeeding do not prove this later native lookup/validation safe.

The known borrowed string_view cache issue is not automatically the explanation: this native path calls owning to_wstring, not to_const_wstring. The read-only parameter contracts and current-thread comparison passed on the first load. Neither those observations nor a successful compile proves reload safety. Do not change the framework, receipt core, scheduler, save isolation or grant behavior based on these hypotheses.

## Prepared diagnostic, not installed or armed

Before editing, the failed source, build configuration, binaries and complete stage were copied and hash-verified in `backups/native-host-reload-before-diagnostics-20261031`, with restoration instructions. The original failed stage is retained.

The candidate adds bounded checkpoints before/after path conversion and exact lookup, then reports the index read, array count, object slot, validation result and class-check entry. The lookup/call sequence, thread restrictions, event cap, title refusal and read-only scope are preserved. It does not claim to fix the crash or catch and continue after a native access violation.

Candidate stage: `build/ue4ss-native-host-reload-diagnostic-20261031`. Standard MSVC build/link, native policy tests, file-only import checks and Lua boundary tests pass. No game/framework DLL was loaded into a tool process. Working APWorld/transports and save files were not modified. No installer, expiry, watcher or test is running.

## Next test, only when the user returns

1. Keep the AP client closed. Recheck the exact game process is closed, stage and executable hashes, and original-file inventory. Take a fresh verified recovery backup before explicit installation.
2. User launches manually and waits at title. Inspect construction, Unreal readiness, Lua binding and hook registration before asking them to load.
3. Load development slot 62 and wait with control. Require completion of all native checks and conversions. No combat, chest or manual save is needed.
4. If the first load succeeds, return to title and reload slot 62 once. Inspect the exact checkpoints immediately. If a fatal error occurs, close normally and do not relaunch or repeat the same candidate. A screenshot is useful if available but not required.
5. After the user closes the game, archive the log and compare closed recovery copies with the fresh pre-test backup. Remove only unchanged manifest-owned additions. Never restore saves automatically.

A successful reload and normal close are required before preparing an installed receipt bridge. The already tested C receipt adapter remains the offline delivery baseline; no UE4SS item grant is authorized by this probe. Further integration can preserve that adapter while replacing only discovery and reflected-call plumbing once this host is reliable.
