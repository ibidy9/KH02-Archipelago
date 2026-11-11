# Attachment failure investigation

A public tester reported `Native ARM not confirmed`, followed on reconnect by an unknown slot-1 snapshot. The original error does not distinguish timeout, Windows message failure, process exit, invalid acknowledgement, or native setup rejection. No tester evidence yet establishes which occurred. A first launch creating an unassociated container is a plausible explanation for the reconnect failure, not a proven diagnosis. Do not delete or adopt saves as a workaround.

## Prepared diagnostic change (not yet live-tested or published)

- ARM waits up to 30 seconds for the same request, instead of two seconds. It is never resent. Other command deadlines and receipt validation remain unchanged.
- The serializer records elapsed time, response sequence, returned receipt, Windows posting error, and whether the original process exited.
- Startup errors distinguish timeout, posting failure, process exit, native rejection, and mismatched acknowledgement. A native rejection includes numeric status and active/restoration fields; it does not yet identify an internal native setup stage.
- The coordinator writes `attachment-failure.json` inside the profile session directory before cleanup. It prints that path. Diagnostic write failure does not replace the original error or skip cleanup.
- Native DLLs and PAKs are unchanged. The public release and installed client are unchanged.

Offline validation: six virtual-clock tests cover delayed acknowledgement, timeout, immediate rejected status, Windows post error, process exit, and unchanged non-ARM deadlines. All 11 source-package tests passed after packaging. These checks do not establish that the tester's original failure is fixed.

## Launch-order findings

`steam_platform.running_games` filters the exact KH 0.2 process basename. An unrelated Steam game is not admitted or counted as KH. `StorageSession.prepare` checks before, immediately before publication, and after publication for a running KH process. Startup admission also validates the prepared process's storage evidence. Launching KH too early should be refused separately; it is not established as the reported ARM failure's cause. The 2.8 launcher can be open without 0.2 running.

## Evidence and next step

Obtain the tester's full client log and private profile session evidence. If reproducing with this diagnostic build, keep KH closed during preparation and wait for attachment at the title screen. On failure close the game normally while retaining the client for cleanup, and preserve the printed diagnostic file. An existing unknown snapshot still requires investigation; this patch deliberately does not reinterpret it as empty history.

Workspace recovery: `backups/attachment-diagnostics-20261020` contains verified pre-change Python sources, the accepted APWorld (including binaries/configuration), hashes, and restoration instructions. Candidate artifact: `build/attachment-diagnostics-20261020/kh02.apworld` at the workspace root. No live test is armed and no save or game installation was changed.
