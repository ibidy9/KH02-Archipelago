# UE4SS direct reads passed on load and reload

The user loaded development slot 62, returned to title, reloaded 62, and closed normally. Both native getters returned successfully on both loads. The adapter matched the current local controller and possessed Aqua; copied owning paths agreed on /Game/Levels/dw/dw_08/dw_08. Getter flags and named parameter contracts passed before calls. Both title callbacks were refused as non-Aqua.

First controller/pawn: 0x2034C9C73C0 / 0x20323E0A060. Reload controller/pawn: 0x203450BE680 / 0x203320BA060. Distinct objects were resolved afresh; no engine object was retained by the Lua modules across callbacks. No call error or map error was logged.

This proves the direct exact-UFunction read path in this session. It bypasses the failed UObject.CallFunction route and no longer uses the temporary FString level-name return. It does not prove that all framework defects were fixed, establish game-thread delivery authority, or authorize an item grant. The three proposed framework patches were not applied; published UE4SS runtime bytes remained unchanged.

Evidence: workspace research/ue4ss-direct-read-live-20261031, with original log, source/binary/config archive, manifests, closed profile copies and decoder reports. Actual pre-test backup and post-close copies were both decoded using the existing verified decoder. Only slot 0 changed; every manual slot, including 62 and 64, was unchanged. No recovery copy was restored.

All ten added probe files were hash-verified and removed with the game closed. Existing Microsoft XInput was preserved. The removal record is in build/ue4ss-direct-read-20261031/installed.json and the capture's removed.json. No test remains armed. Working APWorld/native runtime and production receipt/save systems are unchanged. The fresh recovery backup path is recorded in installed.json.

## Next work

Use this observed reflection contract for the native host boundary. Preserve the existing receipt/session and inventory checks in the prepared adapter. A compatible synchronous host and actual thread/owner validation remain required before any reflected item grant. Do not repeat another map-only or possession-only test without a new concrete question. The next meaningful delivery milestone should bundle one received item, duplicate refusal and save/reload once that host is prepared and checked offline.
