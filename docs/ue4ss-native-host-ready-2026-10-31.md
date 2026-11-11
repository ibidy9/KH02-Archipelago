# Native host: original preparation (superseded test status)

**Update:** The first load passed, but reload caused a fatal error. The probe was removed. Read `ue4ss-native-host-reload-investigation-2026-10-31.md` before using these original preparation notes. A new diagnostic is staged only; nothing is installed or armed.

The game remains closed. No probe, installer or watcher is armed; previous additions remain removed. This preparation did not modify saves, the working APWorld/native transport, production source, public repository or release.

A standard C++ UE4SS mod now compiles and links against the actual SDK using workspace-local MSVC 14.38, /MD and the supported release layout. It exports only start_mod/uninstall_mod. The published framework DLL and proven 0.2 layouts are unchanged; proposed framework fixes are not applied.

Prepared observations combine native mod loading, native callback/window-thread comparison, independent C++ player/controller getter calls, and optional read-only Potion/Hi-Potion name conversion. Exact function flags, parameter count/size/return offsets and field types/offsets guard the calls. Lua addresses are compared with fresh named objects, never dereferenced directly. No object is cached across events and no award function is called.

Source: experiments/ue4ss-native-host. Build/import evidence: workspace build/ue4ss-native-host-20261031. Stage: workspace build/ue4ss-native-host-probe-20261031. The host is not live-proven and provides no AP authority.

Offline checks pass: native thread/path refusal policy; MSVC compile-time agreement with C receipt/adapter structures; Lua/native argument handling, title refusal and caps; and file-only verification that imported functions exist in the selected runtime providers. The existing 57 receipt/adapter tests remain the delivery baseline. No game/framework DLL was loaded into a tool process. An isolated fixture DLL (not the host) also validates process-lifetime callback retention and confirms the file is unlocked after its test process exits. This guards the framework retaining callback manager code beyond mod destruction; it does not claim normal game close is already proven.

The portable compiler/SDK are confined to workspace tools/msvc-14.38-kh02 and its download cache. Packages were verified against Microsoft's hashes and extracted without an IDE/global PATH change. They are developer tools, not player dependencies. Research retains official manifests, licenses, helper provenance and SDK header revisions. Only PATH/INCLUDE/LIB are retained in the temporary compiler environment record, not the complete process environment.

## Next live test, only after the user returns

1. Verify game/client closed, executable/stage hashes and target inventory; create a fresh verified recovery backup before installing eleven absent additions. Reuse existing identical XInput.
2. User launches manually. At title inspect CONSTRUCTED, UNREAL_READY, LUA_BOUND and Lua REGISTERED before loading. Missing binding/load error stops here.
3. Load development slot 62, stand with control. Require native same-thread evidence, current controller/pawn matches and Lua native_accepted=true. Conversions are separate observations; no inventory/popup change is expected.
4. If successful, title/reload 62 once for fresh objects and native thread confirmation. On failure retain explicit diagnostics instead of repeating an uninformative load. No combat, chest or manual save.
5. User closes normally. Preserve logs/source/binaries/config and closed profile copies. Decode copies against the actual fresh pre-test backup; compare all manual records and remove only unchanged owned additions. Never restore saves automatically or overwrite 62/64.

The stage has installed=false, armed=false and no expiry. Nothing is waiting on a live timeout.

A native host pass permits preparing the synchronous C ABI receipt bridge; it does not authorize grants. Preserve the existing owner, receipt memberships, lease/inventory checks and early storage isolation. After that bridge is implemented/tested offline, bundle a first item, duplicate refusal and save/reload into the delivery milestone. Scheduler replacement and explicit persistent installation/inactive PAK behavior remain separate work.
