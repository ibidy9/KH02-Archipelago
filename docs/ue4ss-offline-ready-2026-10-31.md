# UE4SS offline checkpoint: ready for a read test, not armed

This preparation checkpoint is superseded by the successful live result in `ue4ss-direct-reads-passed-2026-10-31.md`. The probe has since been removed; no test is armed.

## State

The game is closed. All prior probe additions were removed and verified. Existing Microsoft XInput was preserved. No AP startup intent is present. The working SQLite-fixed APWorld, production native source and development saves were not changed by this offline work. No installer, watcher, background controller or live test is armed. No commit was pushed and no release was changed.

## What is now prepared

- Direct exact-UFunction reads, avoiding the source-level UObject.CallFunction marker mismatch. Getter flags and field types/offsets are checked before calls.
- Map identity from matching copies of player/controller owning paths; no temporary FString level-name call.
- A read-only single-load probe with call-by-call diagnostics, explicit refusals, a 32-event cap, no timers and no deadline. Stage: workspace build/ue4ss-direct-read-20261031. Source: experiments/ue4ss-direct-read.
- A separate native converter/grant adapter that retains the production receipt/session state machine. Its host/thread checks are injected for testing, not integrated into the game. No game-facing grant is armed.
- Three proposed framework source patches and standalone regressions, with source hashes/licenses. No upstream checkout or runtime binary was patched.

## Evidence / classifications

Live-proven: framework startup, first context probe's two valid local-Aqua possessions, named item functions and six parameter descriptors, exact/dynamic lookup equality before/after metadata on the later capture, and the owning dw_08 paths. Separate live captures also contain lookup and call failures; success in one launch is not treated as universal reliability.

Offline-reproduced source issues: mismatched function-wrapper marker; borrowed string-view cache keys; function-valued Lua error when a C++ exception crosses the Lua boundary. The proposed boundary translation preserves a real message/traceback in pinned C++ Lua. Static inspection also identifies the temporary FString return-wrapper lifetime problem; the reader avoids it. Exact cause of the earlier missing dynamic getter remains unproven.

Behavior-preserving production changes: none. Source patches are compatibility candidates requiring a matching rebuild and live validation. The new Lua/native adapters are experimental, with offline evidence only. No proven gameplay system was replaced.

## Verification

Fifty-seven receipt/adapter tests pass using the unchanged production receipt/session core, including all seven consumable parameter layouts, duplicate refusal, original guard preservation, changed context during conversion, receipt revocation, no/mismatched inventory effects, and a failed grant call after an effect (uncertain, never automatically retried). Normal test artifact has no enabled live hook or fault export.

Lua reader, map parser, metadata checks, bootstrap cap/failure handling and the full simulated load/reload path pass. Both language-mechanism C++ tests and the pinned-LuaRaw error regression pass. Proposed patches pass application checks without being applied. Artifacts: workspace build/ue4ss-binding-audit-20261031 and build/ue4ss-reflected-grant-offline-20261031.

Limits: mocks and isolated native tests do not prove real UFunction invocation, host ABI, actual game-thread ownership, or save-isolation/UE4SS coexistence. No item was awarded through UE4SS. Avoid claiming those milestones complete.

## Next live test, only after the user returns

1. Keep AP client closed; verify game closed and take a fresh verified profile backup. Review the staged manifest before installing only absent additions; reuse identical XInput.
2. User launches manually through Steam/2.8 and waits at title.
3. Load development slot 62; stand with control. Require CALL_END for both object getters, ACCEPT, matching current controller/pawn and dw_08 owning path. A refusal must identify the operation/field; registration alone is not a pass.
4. If successful, return to title and reload 62 once to verify new objects. If failed, do not repeat an uninformative load. No combat, chest, manual save or Main Road run.
5. User closes normally. Archive log, manifests and closed profile copies; decode only the copies, compare records (preserve 62/64), remove only unchanged owned additions. Never automatically restore saves.

No steps above are armed or installed at this checkpoint. The manifest has no expiry, so the user can return later without a timeout.

## After the read boundary

Choose and validate a compatible native host/thread interface before wiring the converter/grant adapter. Retain the current owner thread, receipt membership/lease sequence and inventory checks. Then test one received item, duplicate refusal, save/reload and failure recovery as a bundled delivery milestone. Keep early save isolation and all other proven systems intact. Explicit persistent mod installation/inactive PAK behavior remains a separate integration requirement, not something the loader probes have resolved.
