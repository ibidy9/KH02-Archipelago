# KH 0.2 code-review cheatsheet

A refresher for the implementation paths we reviewed. For the longer reading guide and coverage record, see [CODE-WALKTHROUGH.md](CODE-WALKTHROUGH.md); for source locations, see [SOURCE-MAP.md](SOURCE-MAP.md).

## The four pieces

| Piece | Job |
| --- | --- |
| AP world | Builds the item pool, locations, access rules and seed settings. |
| Python client | Connects to AP, selects the seed profile, coordinates delivery and reports checks/goal. |
| Installed mod | Redirects saves early, observes the game, applies items and enforces gameplay rules. Includes UE4SS and preserved game-specific native code. |
| PopTracker | Displays received items and checks using its exported copy of access rules. It does not deliver items or authorize saves. |

**Which source is active?** Packaging starts with `payload/client/kh02`, then replaces same-named files with `integrations/ue4ss/client`. Read the override where one exists. The release verification report records this mapping.

## 1. Starting a session

`AP launcher → unpack verified client → connect to server → identify seed/player → prepare profile → user launches game → verify storage → ARM → active service`

- `worlds/kh02/launcher.py`: verifies and extracts the bundled client, then starts it.
- `steam_client_v187_candidate.py`: coordinates the client lifecycle; the effective version is under `integrations/ue4ss/client`.
- `steam_discovery.py`: locates the relevant Steam/game installation. It does not need unrelated games or documents.
- `steam_launch_v187_candidate.py`: prepares the session, checks the supported executable and manually installed mod.
- Startup native code redirects save access before the game's initial save-data check.
- **ARM** is the request to admit this specific session to native operations. The native side validates identity and state, then acknowledges it. “Prepared” is not “attached,” and “attached” is not proof that a loaded save has an admitted owner.

The exact game build matters because some native operations depend on its layout. That is different from unnecessarily requiring one exact Windows SQLite version.

## 2. Files and saves

| Area | Why it exists |
| --- | --- |
| Game mod files | User-installed loader, UE4SS components and two paks. They stay installed after a session. |
| `%LOCALAPPDATA%/KH02AP/client-cache` | Extracted client code. |
| `%LOCALAPPDATA%/KH02AP/runtime` | Native runtime components used by the client/mod. |
| `%LOCALAPPDATA%/KH02AP/profiles` | Seed/player saves, associations, receipts, preferences and diagnostics. |
| Session records in the game directory | Temporary authorization/readiness data, cleaned up with ownership checks. |

A profile belongs to a **seed, team and AP player slot**. A manual game-save slot is a separate concept: slot 1 in two seeds is not the same run.

A save association connects a recognized game snapshot to its receipt history. An unknown save must not be assumed to have an empty history. The explicitly owned empty first-boot container has its own admission path.

**Recovery rule:** preserve the whole profile. Do not delete individual receipt/history files to bypass an error, adopt vanilla saves, or mix files between seeds. To play vanilla, follow the installation sheet's removal instructions for both the paks and loader DLLs; removing only the paks is insufficient.

## 3. Receiving one item

`AP ReceivedItems history → receipt policy → delivery request → native validation → game operation → acknowledgement/receipt → notification`

| Module | Responsibility |
| --- | --- |
| `receipt_policy.py` | Compares ordered server history with the admitted save branch and plans missing receipts. |
| `item_delivery.py` | Serializes delivery and resolves native acknowledgements. |
| `shared_client.py` | Coordinates delivery, checks, goal reporting and notices for the current owner. |
| `notification_delivery.py` / `notification_queue.py` | Manage notifications separately from item ownership. |
| UE4SS `bridge_dispatch.hpp` and registration code | Validate the engine receiver and dispatch supported operations on the appropriate game-thread path. |

The item ID determines the effect; a Potion is one argument, not a separate delivery system. Consumable awards, wardrobe/map registrations and ownership policies use their appropriate handlers.

**Duplicate prevention:** repeated server packets are not new awards. Receipts, sequence numbers and save associations distinguish already processed items from missing ones. A visible popup is not proof that the item has been saved to disk.

**Timeout rule:** an unacknowledged operation may already have acted. Do not blindly award it again. Stop or reconcile through the guarded path.

## 4. Loads, Continue and Retry

Loading changes which game snapshot and receipt branch are current. The client/native owner must reconcile that state before further delivery. A raw pointer or an old acknowledgement does not establish ownership of the new load.

Retry and Continue can restore earlier game state. The lifecycle code reconciles the restored branch with received history. Persistent ability/stat ownership is reapplied; consumables use receipts so reapplication is not an unlimited refill loop. Native save commits and the client journal must agree.

## 5. Abilities, magic and stats

- AP ownership is the source of truth for randomized abilities. Gates are reapplied across loads and transitions instead of trusting tutorial unlocks.
- Progressive elemental magic gives the regular spell and then its grand magic. Ownership, cast permission, menu greying and situation-command availability are related but distinct checks.
- Spellweaver and grand magic must enable the necessary situation-command support without waiting for a Castle gear.
- HP/MP bonuses affect maximums. Restoring current HP/MP after game resets must distinguish a transition reset from actual damage or spent MP; it must not heal the player on every update.
- Mickey's YAML mode controls vanilla behavior, a Depths-only unlock, or an unlock usable in other worlds. Catch-up behavior helps where native navigation cannot follow Aqua.

Most game-specific implementation is in `native/transport/common`; UE4SS supplies selected lookups, lifecycle observations and reflected operations. We did not replace every custom hook.

## 6. Checks, rewards and goals

A **check** reports that a location was completed. Its AP reward can belong to someone else. Suppressing a vanilla reward must preserve the check itself.

- Chest checks and contents are separate; objective completion and wardrobe ownership are separate.
- Repeatable boss consumable drops are not repeated AP locations.
- Notification timing can be deferred past defeat animations or scenes without changing receipt ownership.
- **Goal access** decides whether the ending/mirror may be entered. **Goal reporting** tells AP the chosen final boss was defeated. The server decides whether completion releases remaining items.
- Wayfinders, Zodiac relics and wardrobe sets implement the selected unlock condition. The Wayfinder-only mirror was live-tested without all twelve Zodiacs.

## 7. Travel, restart and focused retry

| Action | Intended effect |
| --- | --- |
| Ordinary return | Return to a discovered save point with the appropriate world state. |
| First travel / world restart | Enter the world's opening route and reset the intended local progression. |
| Focused retry | Replay a specific encounter or sequence; preserve unrelated progression. |

Forest Ivy Rail retry now preserves completed Forest story state. Cross-world retry reconstructs the Forest return state before entering the rail. It must not make final Darkside respawn merely because the rail was retried.

`forest_discovery.h` derives earlier-save availability from later Forest discoveries: Path's End implies Rocky Path and Uncertain Path; Rocky Path implies Uncertain Path. This is a read-only effective mask, not fabricated saved discoveries. A real restart still clears later access as intended.

## 8. Traps, DeathLink and the Mines gate

- Traps are received items with native effects and receipts. Stun can apply during combat; MP recharge temporarily blocks casting. Reload must not repeat an already committed trap.
- DeathLink separates an incoming death from a local outgoing death, avoiding an echo loop. Retry/Continue ownership still applies.
- The Mines mirror requires Doubleflight to prevent trapping the player inside. The scoped interaction disables entry and shows the requirement; receiving Doubleflight restores entry. Other mirrors remain usable.

## 9. Generation and tracker rules

`worlds/kh02/world.py` builds the AP world. `worlds/kh02/data/access-rules-v261.json` holds the shared access catalog; the filename is historical. `tools/export_tracker_rules.py` exports its tracker representation.

Generation must guarantee a beatable placement. Tracker colors can additionally describe difficult out-of-logic attempts. Zodiac Phantom requires **Cartwheel, Barrier and Cure** in generation; the tracker shows yellow when fight access exists but any of that kit is missing. Missing world/entry access remains red.

Tracker markers can appear on overlapping maps while referring to the same location. They do not create extra checks. PopTracker runs local rules; it does not receive every accessibility color from the AP server.

## Support: where to look first

| Symptom | First distinction |
| --- | --- |
| “Prepared” but never attached | Check UE4SS/startup logs and attachment diagnostics. Preparation alone does not prove the mod loaded. |
| Native ARM not confirmed | Read the detailed failure evidence; this is an acknowledgement failure, not automatically a Steam-discovery problem. |
| Vanilla saves visible | Stop before saving. Verify startup redirection and session preparation. |
| Unknown snapshot / unassociated slot | Preserve the profile and inspect associations/history. Never assume empty history. |
| Loaded but gates wrong | Check current owner admission and active state, not just server connection. |
| No popup | Compare item/receipt/check state before concluding delivery failed. Scenes can hide or delay UI. |
| Duplicate-looking message | Distinguish repeated network history, a queued notice, and an actual second inventory award. |

Ask for the client log and relevant diagnostic records. Logs can contain usernames, paths, server/player information or chat; users should review them before public sharing. Never ask for a Steam password.

## Small vocabulary reminder

- **Module:** an importable Python unit. `sys.modules` is Python's registry of already loaded modules.
- **`os.replace`:** replaces a destination file with a prepared file; used to publish a complete update rather than partially rewriting it.
- **`.inc`:** source text included into another C/C++ file, not a separate executable.
- **RVA:** an offset relative to the executable/module base. A long hexadecimal value can also be a mask, constant or pointer bound; it does not by itself mean scanning memory.
- **Pointer validation:** rejects implausible addresses/alignment, but those checks alone do not prove the object is alive or of the expected type.
- **Epoch/cookie/sequence:** identifiers used to reject stale owners or requests and match responses.
- **Receipt:** evidence used to reconcile an item against a particular admitted game/save history.

## What the review covered

We went through the main startup, storage, delivery, duplicate prevention, lifecycle, gameplay, travel, rules and support paths in the actual source. The walkthrough records the scope. It was not a line-by-line review of every helper or third-party dependency. Live tests and offline checks complement that review; neither proves the absence of every bug.
