# Receipt host r2: SDK root filter, existing private save

Update: this test subsequently passed. See `ue4ss-receipt-passed-and-next-2026-10-31.md` for closed evidence and the current r3 reconnect/queue plan. Instructions below are historical; do not reconnect the nonempty profile with the old empty-history fixture.

Status: offline prepared, not installed or armed. This supersedes the next-test instructions in `ue4ss-receipt-bridge-ready-2026-10-31.md`.

## First integration result

Capture: workspace `research/ue4ss-receipt-live-20261031T234018396649Z`. The normal client attached, the shared owner became active, and the user started a private Forest game, saved slot 1 and reloaded it. The new host refused both observations before item release because the optional name lookup `/Script/Engine.Controller` returned null. The Pawn lookup and its current identity/slot checks passed. The game did not crash, no AP item was sent and no grant was entered. Why that SDK class-name lookup failed remains unproven; it is not evidence the game lacks its Controller base class.

Normal coordinator cleanup completed. The private AP profile, three journal associations, logs, source/binaries/config and both original/removed manifests are preserved. All eleven unchanged UE4SS additions were removed. The exact previous stable runtime and metadata were restored under the normal exclusive RuntimeLease. All **100 development save records**, including 62/64 and autosave 0, were unchanged. No save was adopted, deleted or restored.

## Scoped correction

The experimental host now supplies the SDK's initialized `UObject::StaticClass()` as the non-null search class. This removes optional Controller/Pawn class-name queries. The non-null class still bypasses the global name-only cache that caused the earlier stale pointer crash. This is an ordinary exact-name SDK lookup, not a process-memory scan or a manually implemented object registry traversal.

The pinned SDK initializes Object's StaticClassStorage in TypeChecker::store_all_object_types. The host checks that this is a live UClass named Object before using it. The current target's returned address must still match the independently obtained event identity **before object reads**. Exact concrete controller/pawn class checks, independent local-controller/current-pawn getters, thread checks, owner permission, item function contracts, leases, receipt membership and inventory verification remain unchanged. The broader initial type filter does not broaden the final accepted player/pawn contract.

Source audit also found snapshot search pools for UClass and AActor in UnrealInitializer::PostInitialize. Root UObject is not one of those pools, so this choice does not introduce dependence on an actor snapshot pool. The framework's construction hook is disabled in the selected fork; do not switch to AActor::StaticClass as a performance shortcut without auditing pool refresh. The framework itself is not patched or rebuilt here, and its optional named-class lookup is not claimed repaired.

This is a compatibility correction to the experimental lookup helper and requires live validation. It does not replace any proven save/delivery system or relax final acceptance checks. The passed read-only host remains unchanged alongside it. Backup of the blocked candidate's source/tools/binary/config: `backups/receipt-host-before-sdk-root-lookup-20261101`.

## Ready artifacts and save reuse

Stage: `build/ue4ss-receipt-acceptance-r2-20261101`. The artifact suffix is an identifier; no test has started. The client, startup DLLs, PAKs, C receipt adapter and Forest fresh/resume transports remain unchanged from the first candidate. Only the separate C++ host lookup changes. The normal test driver now accepts an explicit `--stage` and a declared private profile root.

This stage intentionally reuses **only** the first test's private AP profile below `build/ue4ss-receipt-acceptance-20261031/profiles`, for seed UE4SSReceipt20261031/player APReceiptFixture. It does not use vanilla slot 1 or the user's playthrough. A read-only decoder and the unchanged resume_records/read_associations path verified the copied slot 1 against exact durable AP associations with empty history: three associations, two admitted distinct records. See the capture's `resume-preflight.json`. This preserves the unknown-snapshot refusal rule; it does not assume an empty history or adopt a save.

Build/import/dispatcher checks and the previous 67 C receipt/adapter/ABI tests remain required. Repeat the new-stage loopback admission/Hi-Potion/Sync check, readiness and cleanup-lifetime tests, archive/hash audit and no-arm check before installation. Retain the correct distinction: BP_TresGetItem's reflected ParmsSize is **17**, its aligned owned C++/C-interface frame is **24**, and ReturnValueOffset is **0xFFFF**. The r2 change does not alter those contracts.

## Next live test, only on explicit user readiness

1. Keep the game and ordinary AP client closed. Verify r2 final hashes, exact executable, existing private profile/journals and known slot 1. Take a fresh verified recovery backup, then explicitly install the r2 manifest additions. The normal controller also snapshots the previous stable runtime/prototype before staging. Do not use a new hook-module path.
2. Run `py -3.13 tools/run_ue4ss_receipt_acceptance_20261031.py live --user-ready --stage build/ue4ss-receipt-acceptance-r2-20261101`. It uses the normal resume preparation and reuses the existing associated private profile. Wait for AP prepared, user launches manually, then verify title attachment and current-process UE4SS readiness.
3. Ask the user to **load private slot 1**, not start another New Game. At the Forest save point inspect current context acceptance and `GRANT_CONTRACT ready=true`. Do not release an item if these fail. No combat, Main Road, chest or extra setup run is required.
4. Use the same driver with `send --stage build/ue4ss-receipt-acceptance-r2-20261101`. Require one Hi-Potion popup/count 1, one native entry/return and exact-inventory receipt success. Then use `sync --stage ...` and require no repeat. Save private slot 1, title/reload slot 1, and require count 1 with no repeat grant/popup.
5. User closes normally; wait for the controller to exit and coordinator cleanup to finish. Archive the r2 log, private profile/journals and vanilla development profile before removing only unchanged owned additions. The session profile is under the **declared reused profile root**, not r2/profiles. Restore the saved pre-test stable runtime/metadata with the game closed and exclusive lease held. Preserve unknown changes and every save.

Do not reuse the first stage's stale final-verification hashes after driver changes. Its full original archive remains in the closed capture. r2 has its own hashes; neither candidate is a public package or a live-proven UE4SS delivery solution yet.

## Completed offline verification

The r2 stage passed all 67 receipt/adapter tests, 5 readiness tests, 2 cleanup-lifetime tests, exact loaded-module/Unicode/decoy tests and real loopback AP admission, Hi-Potion history and repeated Sync. Live mode without explicit readiness refused before arming. Current native source hashes match build metadata and staged binary; current tool sources are archived in the probe ZIP. `final-verification.json` freezes these artifacts. The exact associated private save hash and original stable runtime were rechecked; staged additions remain absent from the game directory. The closed first capture has a verified SHA256SUMS.json. No live grant is proven and r2 is not installed or armed.
