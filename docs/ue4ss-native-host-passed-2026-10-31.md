# Native host load/reload passed

This supersedes the untested status in `ue4ss-native-host-fresh-lookup-ready-2026-10-31.md`. The native host remains an experiment, separate from the working AP package.

## Live result

Workspace evidence: `research/ue4ss-native-host-fresh-lookup-live-20261031`. Fresh pre-test recovery backup: `backups/before-ue4ss-native-host-fresh-lookup-20261031-20261031T221741Z`. Both include configuration/source/binary or save recovery material with hashes. The capture retains original/removed manifests, full UE4SS log, verified copied save containers, decoded before/after records and result.

The user loaded development slot 62, returned to title and reloaded slot 62, then closed the game normally. Both observations completed with native_accepted=true. The actual callback thread and own-process window thread matched on both observations (37392).

First controller/pawn: 0x234D1E35A40 / 0x2343A960100. Reload controller/pawn: 0x234BC87F340 / 0x234B09540C0. All four class-filtered actor lookups matched the independently obtained event identities before object reads. Native controller/pawn getters matched. Both Potion and Hi-Potion name conversions completed on each load. The title contexts were refused as non-Aqua.

Unlike both earlier untyped probes, the reload completed without the fatal error. This supports the typed cache bypass and identity-first validation correction. It does not prove why framework deletion invalidation failed, or repair other consumers of its global name-only cache. Do not reintroduce untyped lookups for map actors.

## Cleanup and save protection

After confirmed process closure, the log and closed profile were preserved. All eleven unchanged manifest-owned additions were removed. Existing XInput was preserved and verified. No game launch/close, manual save, save edit, save restoration, AP grant, owner attachment or production package change was performed by the tools.

Both copied save containers passed integrity verification. Only autosave record 0 changed; every manual record, including 62 and 64, remained identical to this test's actual fresh pre-test backup. No test is installed or armed.

## What this unlocks

The standard C++ mod ABI, Lua/native callback, game-window thread comparison, fresh typed actor resolution, guarded native object-returning ProcessEvent calls and read-only name conversion have now passed this load/reload scenario. No UObject is retained across events. This does not establish receipt authority or prove a UE4SS item grant.

Next offline work is a versioned plain-C bridge to the already tested reflected_grant.c adapter. Preserve the proven shared owner, memberships, leases, inventory reads, uncertainty handling and sole receipt acknowledgement. Retain the existing Windows-message executor initially; do not infer that CppUserModBase.on_update is the game thread. Keep early storage isolation unchanged. The first grant milestone should combine a new item, duplicate refusal and save/reload, after its offline checks, with explicit user participation.
