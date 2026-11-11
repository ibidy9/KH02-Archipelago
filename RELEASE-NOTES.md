# KH 0.2 — Steam UE4SS update

This update moves engine access to the KH-compatible UE4SS framework where practical and makes the game mod a separate, manual installation. The APWorld still includes the client.

## Downloads

- **kh02.apworld** — install through Archipelago Launcher, then restart the launcher.
- **kh02-steam-mod.zip** — with the game closed, extract into the collection's **KINGDOM HEARTS 0.2 Birth by Sleep** folder, merging Binaries and Content.
- **kh02-poptracker-0.4.13.zip** — optional; put the ZIP in PopTracker's packs folder.
- **PLAYER-INSTALL.txt** — setup, shutdown, vanilla switching, and save-backup instructions.

**Steam open → connect a fresh KH client → wait for “AP is prepared” → launch 2.8 and select 0.2 → wait for attachment.**

The mod stays installed between sessions. Close the game before the client. To play vanilla, follow the README's instructions for moving the two AP paks and two loader DLLs out of the game directory. Removing only the paks is not enough.

If upgrading from the older automatic-install build, close it normally and let it finish cleanup before installing this version. Keep backups of existing AP profiles as whole folders. Do not delete individual save or receipt files to resolve errors, and do not overwrite another mod's files blindly. Continuation of arbitrary old-version seeds has not been exhaustively tested; a new seed is recommended for testing this update.

## Changes

- Manual mod setup with clearer startup, shutdown, and troubleshooting messages.
- UE4SS-backed engine access and reflected item awards, preserving seed-specific saves and receipt-based duplicate prevention.
- Earlier Castle gear and Phantom check reporting; Mines entry requires Doubleflight and explains why it is locked.
- Zodiac Phantom generation logic requires Cure, Cartwheel, and Barrier. Tracker access remains yellow when the fight is reachable but the kit is incomplete. Existing generated placements do not change.
- Wayfinder-only boss-rush entry verified without Zodiac relics.
- Ivy Rail Retry preserves completed Forest progression, including when entered from another world.
- Later Forest save discoveries make earlier destinations available, even if the first save point was skipped.
- PopTracker 0.4.13 includes consistent markers on overlapping Forest maps.
- Readability cleanup and documentation of the main code paths reviewed by the maintainer.

## Testing and limitations

The previous build completed a seeded playthrough. This update has partial playthrough coverage and focused live checks; it is not a claim that every seed or PC has been tested. Final Forest return, retry and menu checks passed with no repeated AP rewards and normal cleanup.

Windows/Steam only; EGS remains deferred. Objective 51 is excluded and cannot be completed here. Demon Tide is only accessible with the Demon Tide goal. Mickey may navigate awkwardly outside Depths or remain outside some boss barriers. Very long notification text may extend beyond its background.

## AI disclosure

OpenAI Codex generated most of the implementation and assisted with investigation, debugging, and test tooling. The maintainer designed the randomizer settings and logic, performed extensive in-game testing, and read and reviewed the main implementation paths. This was not a line-by-line audit of every helper or third-party dependency. Review scope is recorded in docs/CODE-WALKTHROUGH.md.

See the README for features, credits, licensing, and issue reporting. GitHub's automatic Source code archives are for development, not player installation.
