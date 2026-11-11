# Client integration candidate

This directory contains the UE4SS client overrides. The public payload remains
unchanged until acceptance. Packaging copies these files into `client/kh02`;
it does not generate Python edits. Existing module names are retained so callers
do not need a second import path or a new layer of forwarding wrappers.

## Following a session through the code

1. `steam_launch_v187_candidate.prepare_launch` requires a closed game and the
   supported executable, then calls `installed_mod.verify_installed_mod`.
2. That verifier reads the packaged manifest and checks the explicitly installed
   files. A missing or different file stops preparation before creating a profile
   or publishing readiness. It does not download, copy, repair or remove code.
3. The original launcher binds the authenticated seed to its profile, checks save
   associations and creates the fresh/resume payload. These decisions are unchanged.
4. `preinstalled_startup.PreinstalledStartup` verifies the startup DLL but removes
   it from the set of files the session is allowed to delete. The original
   installation transaction now permits only the two session-readiness files.
5. The original coordinator waits for authenticated startup, runs the shared
   owner and delivery service, and waits for normal game exit before cleanup.
   The coordinator also handles process startup/reconnect boundaries and preserves failed-query diagnostics. See the current RC2 review record rather than the original installation-only comparison.

## What is installed or written

| Files | Owner and behavior |
| --- | --- |
| Startup DLL, UE4SS loader/framework/config, native host and Lua scripts | Explicit mod installation beneath `Binaries/Win64`; client verifies and leaves them installed. |
| `kh02-ap-ready.bin`, optional `kh02-ap-resume.bin` | Per-launch client data; the session removes its own unchanged files after game exit. |
| Restart/menu paks | Manually installed under `Content/Paks/~mods`; required and verified before session preparation. The client never installs, replaces or removes them. |
| Isolated saves, receipt journals, preferences and evidence | Profile data retained after closing. The client does not delete saves. |
| Stable native transport cache | Existing staging behavior is retained. Moving its lifetime into UE4SS is separate work, not implied by explicit startup installation. |

The manual-mod candidate archive uses one game-root-relative `Binaries/Win64`
tree. It is not seed-specific. Do not tell users to overwrite another loader or
mod: conflicts need review. This is not yet a final public installation guide.

## Manual installation policy

The user selected ordinary manual mod installation. One archive contains
`Binaries/Win64` and `Content/Paks/~mods` relative to the 0.2 game directory.
Players remove the AP mod files themselves to return to vanilla; no client
uninstall or automatic pak cleanup is provided. Existing unrelated mods must
not be overwritten. The mod archive is not seed-specific.

For AP play, prepare the client session before launching 0.2. If the game was
already running, close it normally and prepare again. Do not describe a launch
without the client as clean vanilla while the AP paks remain installed: their
level overrides still load. The existing bootstrap chooses vanilla storage
without valid AP intent; blocking that startup entirely would be a separate
untested change, not something this packaging change implements.

A client session still writes and removes its own readiness records and retains
save/receipt data. This is distinct from deleting user-installed mod files. The
stable transport cache remains an explicit, documented client-managed component;
this change does not claim the client has stopped writing all executable files.

## Evidence and limits

Fresh and resume startup passed live in r10 and r9 respectively. The engine
binaries in the client candidate are unchanged. Installation validation has
temporary-directory tests for missing/mismatched files, traversal/aliases,
duplicate/incomplete manifests and invalid hashes. Separate ownership tests
cover cleanup and preservation. Frozen comparison reconstructs the passed
launcher after removing the preflight and the passed coordinator after reverting
two messages. Manual-pak ownership has separate18 offline tests and awaits live acceptance. This does not claim every gameplay system has been rewritten or
every starting-world core has been migrated.

## Current acceptance

The historical candidate discussion above records the original integration boundary. Manual installation, fresh/resume sessions, ordinary receipts and saved reloads have since passed focused live tests. The current packaged sources are mapped in `docs/steam-ue4ss-rc2-review.md`; that record supersedes old pending-test statements. This does not mean every native system was rewritten through UE4SS.
