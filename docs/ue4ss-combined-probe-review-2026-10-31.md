# UE4SS combined read-only probe review

The game was closed normally. All nine added probe files were hash-checked and removed. Existing XInput was preserved. Working AP binaries were not replaced. No test remains armed.

## Proven

Six exact parameter descriptors were captured at startup:

| Function | Parameter | Type | Offset |
| --- | --- | --- | --- |
| BP_TresGetItem | WorldContextObject | ObjectProperty | 0 |
| BP_TresGetItem | InItemKeyName | NameProperty | 8 |
| BP_TresGetItem | bShowInfo | BoolProperty | 16 |
| BP_TresConvertBattleItemIDtoKeyName | WorldContextObject | ObjectProperty | 0 |
| BP_TresConvertBattleItemIDtoKeyName | InBattleItemID | ByteProperty | 8 |
| BP_TresConvertBattleItemIDtoKeyName | ReturnValue | NameProperty | 16 |

Static constructor RVAs 0x8204D0 and 0x81F9A0 independently supplied these parameter names. Grant flags were 0x4022401; converter flags 0x14022401. No converter or grant was invoked.

Both title callbacks were refused as non-Aqua. Both slot-62 callbacks reached the Aqua check, then errored at dynamic GetPlayerController invocation. Map-name and object-path diagnostics were not reached. A successful earlier context probe remains separate evidence, not a guarantee across launches.

Closed profile copies passed the existing save decoder's native digest/container checks. Comparison against the immediately preceding decoded profile changed only slot 0 (autosave); slots 62 and 64 and all other manual records were unchanged. No recovery copy was restored.

## Newly implemented, offline only

A separate read adapter resolves fully qualified functions and uses UE4SS's explicit CallFunction entry, with wrong/missing function refusal and no writer operations. Its context reader records owning paths before reads and preserves them on lookup failures. A grant-contract checker verifies captured metadata without granting. Offline tests pass; live behavior remains unproven. See experiments/ue4ss-adapter/README.md.

## Unknown / not yet fixed

A non-callable UObject wrapper is consistent with the fork's missing-function result. The capture does not prove whether that wrapper was null, whether the additional metadata reads contributed, or which field/layout/name traversal caused the lookup failure. Exact-function calls are a candidate bypass, not a proven root-cause fix. Do not release a migration based on mock tests.

## Recoverability / evidence

Workspace evidence: research/ue4ss-context-details-live-20261031, including original log, copied installation manifests, result.json, copied closed profile and decoded report. Backup: backups/before-ue4ss-context-details-20261031T093524Z, including original profile, manifest hashes, probe source/binaries/config archive, working SQLite-fixed APWorld, and restoration instructions. Only unchanged manifest-owned additions were removed. User participation is not needed for continued offline work.
