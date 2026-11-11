Update 2026-11-11: Wayfinder mirror passed; completed-Forest rail preservation passed and integrated. See `forest-rail-story-integrated-2026-11-11.md` for scope, recovery and remaining cross-world limitation. No test armed. Historical runbook follows.

# Remaining review checks — prepared, not armed

Do not install, publish startup authorization, start a live server/client, or ask
the user to launch until explicit readiness. No observation timer is running.

Use `build/review-cleanup-20261110/candidate-final/delivery`. Verify its output
hashes before installation. At readiness, first confirm game/client closed and
back up the installed APWorld/mod/configuration with hashes and restoration
instructions. The candidate keeps the accepted mod/native binaries; do not
install the separately compiled diagnostic binaries. Preserve all saves.

Two isolated ordinary AP room archives are generated under
`build/review-cleanup-20261110/rooms`. These are new seed identities, not copies or
adoptions of vanilla/playthrough saves. Their eight precollected items provide
Cartwheel, Barrier, Cure, Doubleflight, Air Slide, Spellweaver and two Progressive
Thunder items. Use Beginner for the test if desired. No traps or DeathLink.

## First: Wayfinders without Zodiacs

- Room directory `ReviewWayfinders`; actual slot name `ReviewWayfinders`; seed
  `91654466086152336615`. Starts in WW, boss-rush goal, Wayfinder requirement.
- Start its server only when ready, retaining its normal server save/history
  across any reconnect. Do not reuse the offline protocol test's temporary
  in-memory history or start an existing saved profile with an empty server.
- User launches manually after client preparation, waits for attachment, starts
  a new game and stands in the hub. Confirm current owner and initial history.
  Do not collect Zodiac chests or unrelated checks before the mirror observation.
- Send Terra's and Aqua's Wayfinders through ordinary AP server item delivery.
  Wait for acknowledged ownership and let the user approach the center: mirror
  must remain locked. Do not infer ownership from only the popup.
- Send Ventus's Wayfinder once. Verify the three distinct items, no Zodiac items,
  and unchanged session identity. User walks through the normal center trigger;
  verify scene/mirror appearance and entry. Floor symbols need not light.
- No boss completion is needed. Save/reload at a usable point if convenient to
  combine client rename/regression coverage; do not require a separate boot just
  for another consumable. Normal game-first/client-second shutdown and capture.

If WW story progression unexpectedly affects the trigger, capture that fact
before changing state. The fixture is intentionally fresh and does not fabricate
Phantom completion or Zodiac ownership. Do not make the user explore arbitrary
mirrors merely to repair the setup.

## Second: completed Forest, then Ivy Rail retry

- Room directory `ReviewForestRetry`; **actual AP slot name `ReviewForestRetr`**
  (AP truncated the requested name); seed `66144286956750314311`. Starts Forest.
  Use the actual name from generated metadata, not the directory name.
- Establish completed Forest normally in this isolated seed; retain a reusable
  save at Path's End. This is the longer test, not a quick save-point observation.
  If a verified compatible existing isolated AP profile can avoid the initial
  clear, inspect its whole association/server history first; do not adopt a save
  from another seed to shorten the test.
- Record completed final-Darkside, Path's End discovery, and existing AP checks.
  Select Retry Forest Ivy Rail, complete it, then walk forward through Forest.
  Observe the final Darkside, later triggers, save point and exit; do not replace
  traversal with teleportation. Check no duplicate AP location rewards.
- Preserve completed-location history even if local repeatable drops occur; these
  are different systems. Capture native/client evidence without broad scans.
- Normal shutdown; retain server history and saves, then restore only installed
  test files if needed. Never delete profiles as cleanup.

The temporary offline connection test already authenticated both generated rooms
using the actual packaged client and confirmed item history/Sync behavior. It did
not install or arm anything, and it is not a replacement for these observations.
