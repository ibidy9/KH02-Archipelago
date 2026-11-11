# Fresh actor lookup candidate: original readiness notes

**Passed:** Slot 62 load/reload, both native reads/conversions and normal close passed. Probe removed; every manual record unchanged. Read `ue4ss-native-host-passed-2026-10-31.md` for the authoritative result.

This follows `ue4ss-native-host-reload-investigation-2026-10-31.md`. Neither failed host is a production dependency. Working AP behavior remains unchanged.

## New live evidence

The checkpoint diagnostic again passed the first load of development slot 62, then failed on reload with the dialog “Fatal error!”. Capture: workspace `research/ue4ss-native-host-reload-live-20261031`.

The reload completed copied-path conversion. The untyped controller lookup returned `0x1B4D1285A40`, the same address as the first load. The final checkpoint was CHECK begin for that address; no index result followed. This localizes the failure to the object-index read/checkpoint interval after a stale-looking lookup result. There is no instruction-level crash stack, so the exact failing instruction and why cache invalidation failed remain unproven.

The diagnostic was removed after the user closed the game. All manual save records are unchanged against its actual fresh backup; only autosave record 0 changed. No save was edited or restored. Existing XInput is preserved.

## Narrow correction

The prior helper called live(found) before comparing found with the independently obtained current-event identity. That ordering could read an old cached pointer. The new object_identity.hpp guard compares pointer values first and never invokes object validation for a null, zero-expected or mismatched identity. A native regression supplies an unusable stale address and confirms validation is never called; matching current identity is checked, and failed validation is refused.

Transient controllers and pawns now use StaticFindObject with the stable Engine.Controller or Engine.Pawn class supplied. The SDK's FindObject cache shortcut requires Class == nullptr; supplying a class bypasses that name-only shortcut while retaining normal engine object lookup and class filtering. The returned pointer must match this event's Lua identity before its index, slot or class can be read. No arbitrary Lua integer is cast into a dereferenceable object pointer.

The original untyped helper now accepts only /Script/ paths for stable classes, functions and default objects. It cannot accidentally be reused for a map actor. No framework cache is edited, no object is rooted, no UObject is retained across callbacks, and no framework rebuild is involved.

The bypass was checked against the actual published UE4SS.dll as well as the pinned source. Exported FindObject begins at RVA 0x557AC0; test rcx,rcx at 0x557B0A followed by jne 0x557BBC skips the cache branch for a non-null Class argument. The null-Class branch calls the cached lookup at 0x557B5F and can return its result directly at 0x557BAF..0x557BB2. File-only disassembly is preserved in workspace `build/ue4ss-native-host-20261031/FindObject-cache-gate.asm.txt`. These RVAs describe the selected framework DLL, not the game executable, and are evidence rather than new runtime hardcoded addresses.

This is a compatibility correction to experimental lookup/validation, not a redesign of the proven receipt, owner, storage, gating or delivery systems. It still needs live validation on first load and reload. It does not establish that the framework's cache defect is fully understood or fixed for all consumers.

## Recovery and readiness

Before changes, the diagnostic source/build/stage were copied and hash-verified in `backups/native-host-before-fresh-lookup-20261031`, with restoration instructions. The two failed captures retain their original binary/config archives and fresh pre-test recovery profiles.

Candidate stage: `build/ue4ss-native-host-fresh-lookup-20261031`. Native build/link/import checks, identity/thread/path policy regressions and the unchanged Lua boundary tests pass. The source, archive and staged binary hashes must match before installation. The normal APWorld and its transports remain unchanged. Nothing is installed, armed or running.

When the user is ready: create a fresh verified backup, install the exact manifest additions, inspect title readiness, load slot 62, then title/reload slot 62 once if the first load passes. No combat, manual save or item award is needed. Inspect the FRESH returned matches marker and full context/conversion completion immediately. On any error stop rather than retry the same candidate. After user closure, archive and compare closed copies, and remove only unchanged owned additions.

After a successful reload, continue the native C ABI receipt bridge offline. Do not substitute Lua success for receipt acknowledgement, change the scheduler or early save isolation, or deploy this experimental host into the playable package before equivalent delivery tests.
