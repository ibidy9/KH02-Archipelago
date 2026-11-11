# Explicit installation and UE4SS evaluation

Status: isolated startup probe PASSED on the development computer (initialization marker and responsive title menu); all seven additions were removed and the existing Microsoft XInput DLL was preserved. Evidence: workspace `research/ue4ss-probe-live-20261031`. A separate current-player probe also passed two development-slot loads and was removed; evidence is in `research/ue4ss-context-live-20261031`. It identified fresh local Aqua objects and found both named grant functions. Map labels were empty at possession time, and gameplay thread ownership remains unproven. This remains a proposed architecture, not an implemented AP replacement or a general compatibility claim.

## Working baseline

The SQLite portability candidate passed on computer 2: attachment, active state, fresh game, manual save/reload, chest reporting, Progressive Thunder delivery/unlock, final save, and normal cleanup. Offline inspection validated the slot-1 Thunder receipt and all 11 journal associations. Captures are in workspace `research/computer2-sqlite-fix-20261031`. Preserve this package while evaluating loader changes.

## First milestone: loader only

Use the developer-published `ashandarei/ue4ss-goa` v3.0.4 runtime. Its release notes identify source revision `06dd053d3e537ec2b477ee76337661b15642d012` and the UObject-array listener layout correction plus disabled global StaticConstructObject detour. These corrections are supplied only as references in the starter kit, so an unmodified local fork build is not assumed equivalent. Download hashes match the release checksums and GitHub metadata.

The experiment uses a one-line Lua startup log instead of compiling the starter's C++ mod. This establishes whether the runtime reaches mod initialization without installing development tools. It does not prove C++ plugin compatibility or validate all reflected layouts.

The starter kit's experimental 0.2 profiles override engine 4.17 and put ProcessEvent at 0x1F8, rather than KH3's 0x200. Many inherited fields remain unverified. The borrowed KH3 layout that failed in August is not being reused unchanged. Generic configurable hooks, hot reload, console windows, and full-memory dumps are disabled. No object enumeration, gameplay calls, recurring Lua timer, AP communication, or saves are implemented by the logging script. UE4SS itself remains a native loader with initialization hooks and engine signature discovery; the mod being logging-only does not mean the runtime is inert.

The pinned source reads `mods.txt` inside `ModsFolderPath`; it has no `ControllingModsTxt` setting. The ignored setting from the kit is omitted. Probe files use a dedicated KH02UE4SSProbe Mods directory and will not enable other installed mods.

Workspace stage: `build/ue4ss-probe-20261031`. Verified pre-install inventory and SQLite-fixed package: `backups/before-ue4ss-probe-20261031`. No APWorld or player release is published from this probe. Configs inherited from the kit retain its license separately from the runtime MIT notices and original game-asset ownership.

The stage contains seven absent additions and one identical existing Microsoft XInput DLL. Reuse the existing identical DLL; do not replace/remove it. The DLLs' runtime imports were inspected on disk, and the required Visual C++ runtime DLLs are present on this computer. No compiler, Rust installation, or machine-wide settings change is required for this milestone.

### Next live test

Run on this development computer, not computer 2. With both the client and 0.2 closed, recheck the exact executable hash and destination inventory, then explicitly install the seven staged additions. Preserve the before/after hashes and installation record. Do not install alongside AP's dinput8.dll or session-ready files. Launch through Steam/2.8 manually and stay at the 0.2 title screen. Success requires the UE4SS log and unique `[KH02UE4SSProbe] INITIALIZED` marker plus a responsive title menu. Do not load saves or start a new game. On failure, retain logs/crash details; close manually, archive outputs, and remove only unchanged, manifest-owned additions. Never terminate the game automatically or delete saves. Restore baseline before another AP session.

## Proposed player workflow

1. Install the APWorld and open its client.
2. An explicit **Install AP Mod** page detects the game directory, displays that directory and a short change summary, and offers Install. The user does not repeatedly enter paths. A details view lists exact files and ownership.
3. Installed mod files persist between sessions. Updates and uninstall are explicit actions with recovery records; neither AP connection nor disconnection silently installs/removes executable mod files.
4. Connect with server and slot name. The client selects the isolated seed profile and publishes bounded, authenticated session configuration. UI shows installation readiness, connected seed/profile, and game attachment as distinct states.
5. The user launches the game through Steam/2.8. Session configuration may change automatically as ordinary client operation; executable mod installation does not.
6. Explicit **Restore Vanilla / Uninstall AP Mod** removes or disables only owned mod additions with the game closed. AP profiles remain. Unknown or externally modified files are preserved with actionable explanations.

This workflow must be usable on a standard Windows account. It must not require developer paths, compilers, recurring terminal commands, automatic game launch, or save deletion. File lists can be available under Details instead of filling the normal UI.

## Safety and compatibility decisions still requiring implementation/tests

- Persistent PAKs may affect game behavior even when no AP client is ready. Do not assume that removing or ignoring a session file produces safe vanilla behavior. Define and test an explicit inactive mode before shipping a permanent mod.
- The simplest initial rule is that AP mode requires a prepared client. An unprepared launch must explain the missing connection and prevent accidental AP play with vanilla saves. Decide the exact UI after proving UE4SS startup; do not silently adopt vanilla saves or mark AP active.
- Existing native save isolation runs early. UE4SS initialization may be too late to replace it directly. Retain that implementation until an equivalent early startup mechanism passes fresh launch, resume, missing-client, and failure recovery tests.
- Replacing the Windows message hook with UE4SS dispatch changes threading and lifecycle behavior. This is speculative until a game-thread dispatcher and identity/lifetime checks are live-proven. Do not execute existing gameplay work from an assumed-safe callback.
- Existing item delivery, receipts, stat reapplication, ability/spell gates, world routes, checks, traps, and DeathLink remain the baseline. A loader experiment is not authorization to rewrite these systems or claim they are integrated.
- Connecting first versus launching first must have deliberate behavior. Keep the current documented client-first requirement until game-first behavior is implemented and tested.
- Separate source/runtime provenance, licensing, dependency checks, and asset ownership must remain visible in any eventual release.

After the loader test, choose between using UE4SS for settings/reflection only, using it as the native-core host, or retaining the custom loader with explicit installation. Pick based on demonstrated simplification and reliability rather than framework preference. Present the concrete choice before substantial migration work.

## Combined observation outcome

The follow-up captured six grant/converter parameter descriptors but failed twice at dynamic GetPlayerController lookup before map diagnostics. It was removed after normal close; only autosave record 0 changed. An exact-function read adapter and metadata checker are implemented separately and pass offline tests; they remain unproven live. See docs/ue4ss-combined-probe-review-2026-10-31.md and experiments/ue4ss-adapter/README.md. No AP runtime integration or persistent installation has been enabled.
