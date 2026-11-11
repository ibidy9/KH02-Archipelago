# Mines requirement message — offline candidate, 2026-11-05

Status: compiled and packaged; NOT installed or armed. User must explicitly return ready before any live setup.

## Proven baseline
The automatic Doubleflight gate passed in research/ue4ss-mines-access-live-20261105T232032Z. Mines alone lost its prompt, another intact mirror retained its prompt. Real AP Doubleflight receipt produced its popup, restored Doubleflight and the Mines prompt immediately. Save/reload retained both without duplicate delivery; actual Mines entry and control worked. Preserve that capture. No need to repeat traversal/save/reload solely for this cosmetic change.

Backup: backups/before-mines-message-20261105/working.zip, verified against hashes.json; RESTORE.txt describes recovery. Normal installed RC1 APWorld and host hashes remain restored/verified. No saves changed.

## Candidate
Source: experiments/ue4ss-mines-message/native. Build/package: build/ue4ss-mines-message-20261105. All eight passed transport DLLs are byte-identical to the Mines gate candidate. Only the host changes. No new core export, receipt state, delivery pathway, startup, pak, hotkey or timer thread.

When the authoritative owner says Doubleflight is absent and this host holds the intact Mines reactor disabled, query the exact pawn-to-Mines actor distance through UE4SS. Within 400 Unreal units, show “Doubleflight required” through the native information banner. AutoClose=true. Once per approach; leave beyond700 units to rearm, minimum15 seconds between displays. No banner is requested for another mirror, a shattered mirror, a reactor disabled by vanilla rather than AP, or owned Doubleflight. Leaving WW or ownership activation resets approach state, not cooldown. A signature refusal disables only the explanation, preserving the passed entry gate.

Static, exact-build evidence in build/ue4ss-mines-message-20261105/static:
- TresGameBlueprintLibrary.TresUI_OpenInformation registration RVA0x851750: flags0x04422401, params72; InformationText at0, AutoClose at64. Native thunk0x866b60 consumes a const struct reference plus bool, calls0x459c30.
- TresLocText registration0xa40b80: size64; Namespace FString0, Key FString16, Params TArray<FString>32, LocalizedText FString48. All are checked against reflected metadata before use.
- Text resolver0x566f90 takes LocalizedText directly at+0x30 when Namespace/Key are empty (branch0x5671e9). Native widget0x5b4780 resolves and copies text into UI storage. Static input buffers stay alive for module lifetime; none are passed to a native free/destructor.
- GetDistanceTo registration0x22d8d40: flags0x54020401, params16, OtherActor0, ReturnValue8. Runtime checks names/types/offsets before ProcessEvent.
- Calls use UE4SS reflection on the existing authenticated owner thread, under the existing recursion guard. No new raw function invocation.

Offline verification: host compiles/import-audits; package manifest passes; policy C++ tests cover stay-near/no-spam, leave/reapproach, cooldown, distance bounds and NaN/infinity refusal. Actual isolated AP loopback delivers Doubleflight once and Sync does not duplicate. Fresh MinesMessage seed12420762263808334479 starts WW without Doubleflight. Loopback test has ended, no persistent room is running. Offline proof does not establish the information banner’s visual timing/position or native UI behavior.

## Next live session (only on readiness)
1. Verify game/client closed, back up installed APWorld/host/config/profile for recovery only, then install this candidate and verify manifest. Do not edit any save or use the Test playthrough seed.
2. Start tools/mines_message_room_20261105.py live --user-ready as hidden helper; capture stdout/stderr/PID. It creates an address for player MinesMessage. User launches client/game manually; verify attached at title before New Game. Skip scenes to WW. No fights required.
3. Approach locked Mines. Expect one “Doubleflight required” native banner, no entry prompt, free movement/menu. Stay nearby through its disappearance to check no spam; walk away and approach again after15 seconds to check a reminder. Check one other intact mirror still has its ordinary prompt (do not enter).
4. While at Mines, touch live-controls/send-doubleflight ONCE. Expect the ordinary item popup, Doubleflight, normal entry prompt, no new locked reminder. The old banner may finish its normal auto-close; do not globally close native information and risk clearing another game message.
5. Close normally. Capture logs and test profile. Stop helper, restore hash-verified installed RC1 files. No extra Mines traversal/save cycle required unless new evidence warrants it.

## Separate remaining findings
Last automatic-gate session logged an unacknowledged goal/owner probe around closure (objective_transport probe command3 -> query_overlap_v184); gameplay/receipt evidence passed, clean service close did not. Logs are archived; do not suppress or weaken the acknowledgement guard. This cosmetic candidate does not claim to fix that separate issue.
Phantom1/2 earlier check reporting still needs a fresh-check witness; Gear1 earlier scene timing remains pending. Readable seed labels/Open save folder remain deferred. Original playthrough saves untouched.
