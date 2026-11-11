# UE4SS binding comparison and offline findings

## Live result

Evidence: workspace research/ue4ss-binding-live-20261031. One load of development slot 62. The dynamic member and fully qualified lookup returned the same valid GetPlayerController UFunction, at the same address before and after the six metadata queries. Thus the metadata query did not change resolution during this captured event. This does not rule out all timing or allocation effects across launches.

Controller/pawn full names independently identify /Game/Levels/dw/dw_08/dw_08.dw_08. Empty GetCurrentLevelName need not block future context work. Explicit UObject.CallFunction failed both times, with a function-valued Lua error rather than a useful internal message. No reflected gameplay writer was invoked.

The user closed normally. Ten owned additions were hash-verified and removed; existing XInput was preserved. Closed-copy decode passed native checks; only slot 0 changed compared with the preceding snapshot. Slots 62/64 and all other manual records were unchanged. No save was restored. Backup path is in the archived installed.json; it includes source/binaries/config, working APWorld, hashes and instructions.

## Offline results

Pinned source has a mismatched function-wrapper marker in UObject.CallFunction and a borrowed-key lifetime problem in the string conversion cache. Small C++ regressions reproduce both mechanisms without accessing game memory. These findings are distinct from proof of the precise defect in the published binary. See experiments/ue4ss-adapter/framework-patches for proposed corrections and source hashes. The upstream fork and installed runtime remain unchanged.

The read adapter now avoids UObject.CallFunction and invokes an exact UFunction with an explicit receiver. Wrong/missing wrapper/function/arity refusals remain, and tests ensure the retired CallFunction route is not used. No AP receipt or delivery code was changed. This remains an untested game compatibility change, not a behavior-preserving production refactor.

## Next boundary

A single-load read-only check of the direct-UFunction route can be prepared after the offline review; it is not armed. Independently establish native callback/thread integration before any grant. Keep all receipt/lease/inventory checks and early save isolation unchanged. No repeated map-only observations are needed merely to recover a readable level label.
