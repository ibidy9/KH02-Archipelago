# KH 0.2 maintainer walkthrough

Status: the guided walkthrough of the main implementation paths was completed with the maintainer on 2026-11-10. The review record below identifies its coverage and limits. The lesson outlines remain a reading guide; they are not a claim that every line or helper was reviewed.

## What we are trying to understand

You already understand item pools, locations and reachability. We will spend less time on those and more time on the parts that make a native game cooperate with AP: finding the right game, selecting the right saves before the game reads them, applying each received item safely, and surviving loads, Retry and shutdown.

There are four main pieces:

| Piece | Responsibility | It does not decide |
| --- | --- | --- |
| AP world | Settings, items, checks, access rules and generated placements | Which running Aqua object is safe to modify |
| Python client | Server connection, seed identity, received history, check reporting and session management | That a timed-out native command definitely did nothing |
| Installed mod | Early save redirection, current game objects, native operations and gameplay gates | Which remote player receives an item at a location |
| PopTracker | Displays items/checks and evaluates its copy of access rules | Item delivery or authoritative game-save ownership |

The mod includes both UE4SS facilities and preserved custom native code. UE4SS replaced parts of engine lookup, lifecycle observation and reflected awards. It did not replace the entire game-specific receipt, travel, gate or save system. An exact Steam executable check is still necessary for code that depends on this build's layout; it is different from unnecessarily pinning a Windows system library version.

## Lesson 1: what runs when you open the client

Start in `worlds/kh02/launcher.py`, functions `unpack` and `main`. The APWorld is a ZIP-style package. Its launcher verifies the bundled client's hashes, expands it into the user's KH02AP cache, then runs it using Archipelago's environment. The cache is executable project code, not just artwork; users should know that.

Follow `payload/client/kh02/steam_client.py` into the effective `integrations/ue4ss/client/steam_client_v187_candidate.py`. This starts the server/UI lifecycle. `steam_context_v187_candidate.py` learns the seed, team and player slot from the server and pins that identity. Received items belong to that identity and have ordered receipt indices.

Important source-reading rule: files under `integrations/ue4ss/client` replace same-named files under `payload/client/kh02` when packaging. Read the override when it exists. The RC2 verification report maps those effective sources. Historical version suffixes are filenames used by imports, not an instruction to throw the files away.

Discussion checks: Why must the client know the server's seed before selecting a save profile? Why is a client cache hash check useful, and why is it not a guarantee that the code is harmless?

## Lesson 2: what the client reads and writes

Read `payload/client/kh02/steam_discovery.py`, then effective `steam_launch_v187_candidate.py`, `installed_mod.py` and `preinstalled_startup.py`. Discovery locates the Steam installation/account information needed for this game. Preparation requires the game closed, validates its executable and installed mod, then prepares seed-specific data. It does not launch the game for the user.

| Location | Purpose and ownership |
| --- | --- |
| Steam installation/account metadata and KH executable | Read to identify the supported installation and save identity; inspect the concrete discovery functions together |
| Game's installed mod files | Installed manually by the user; client verifies them and leaves them installed |
| Game directory readiness/resume records | Temporary per-session data, removed only through the session's ownership checks |
| `%LOCALAPPDATA%/KH02AP/client-cache` | Verified extracted client code |
| `%LOCALAPPDATA%/KH02AP/runtime` | Staged native runtime code; manual mod installation did not eliminate this cache |
| `%LOCALAPPDATA%/KH02AP/profiles` | Seed/player profiles, game saves, receipt history, preferences and diagnostic/session records |
| Archipelago logs | Support logs; may include local paths, player/room information and chat, so review before sharing publicly |

`integrations/ue4ss/client/steam_platform.py` queries the specifically named KH process and selected loaded modules. Startup verification reads a fixed exported evidence record, not an arbitrary search through process memory. The mod still executes native code inside the game and the client uses OS process APIs. Describe that honestly rather than saying it has no access to the computer.

Discussion checks: Which files remain after shutdown? Why does removing only the paks not fully disable AP? What should support tell someone who sees vanilla saves?

## Lesson 3: save ownership before the title screen

Read `steam_profiles_v187_candidate.py`, effective `steam_launch_v187_candidate.py`, `steam_preparation_v187_candidate.py` and `title_container.py`. Profiles are scoped to the seed/team/slot, not simply to the visible save-slot number. Slot1 in two seeds is not the same run.

The startup DLL (`integrations/ue4ss/startup/unified_filename.c` plus shared startup sources) redirects the game's save-file access early enough for its initial checking-for-saved-data screen. Preparation publishes a bounded launch authorization; the client subsequently checks evidence that the correct process selected the intended AP storage.

An empty first-boot container is different from an unknown existing save. The former can be admitted only through the recorded owned-title-container path. Unknown history must not be treated as empty: doing so could adopt unrelated progress or redeliver items. Save associations tie recognized game snapshots to receipt history. The native save lifecycle and journal publication must agree; this is more than changing a folder name.

Discussion checks: Why does reconnecting reuse a profile? Why should users move the whole profile when backing it up? Why should support not suggest deleting individual receipt databases to make an error go away?

## Lesson 4: follow one received Hi-Potion

Start with a server ReceivedItems event in the context, then `steam_service_v187_candidate.py`, the shared-client chain, and `item_delivery.py`. That module coordinates general delivery planning. Trace a consumable separately from an ability-policy update.

The planner compares the server's ordered item history with the admitted branch's receipts. A native award request carries identity, sequence and ownership information. The host validates the current controller/pawn and operation contract; only then does the game-thread path call the proven reflected award. Read `bridge_dispatch.hpp`, `registration_policy.hpp`, `registration_host.inc` and the bridge API beside the planner.

A visible popup is useful evidence but is not a receipt commit. Conversely, receiving the same packet again is not permission to award the item again. Save/reload reconciliation uses the saved branch's history. Some effects are reapplied as policies; consumables must not become an unlimited grant loop.

A timeout means the result is uncertain, not that the command failed before acting. Never resend an uncertain grant automatically. Follow the sequence/pending guards and failure path before looking at optimizations.

Discussion checks: Why can the game show a popup without the client being allowed to assume durable saving? How does a duplicate network message differ from loading an older save? Why is a second Potion award a dangerous timeout recovery strategy?

## Lesson 5: loads, Retry and UE4SS

Read `integrations/ue4ss/Scripts/main.lua`, `context_reader.lua` and `reflected_reads.lua`. The ClientRestart hook invalidates stale context and inspects the current controller/pawn. An observed object is a candidate, not authorization to grant items. Follow that distinction into `object_identity.hpp`, `weak_identity.hpp`, `context_lifetime.hpp` and `sdk_owner_pump.hpp`.

A room change or Retry can replace Aqua's object. Reusing the old address because it once worked risks modifying a destroyed or unrelated object. The shared owner maintains the accepted session/load boundary and coordinates policies and item receipts. The preserved native core still handles game-specific travel, stats, gates, and save/Retry behavior.

Use the passed WW travel/Retry captures as examples, but distinguish those observations from claims about every possible transition.

Discussion checks: What becomes stale when Aqua is replaced? What evidence makes a new object acceptable? Which behavior is UE4SS doing for us and which behavior is still our code?

## Lesson 6: follow one check out to the server

Read `level_checks_v186_candidate.py`, `level_reporting_v186_candidate.py`, and `objective_checks_v116_candidate.py`. Native snapshots provide facts; the reporter interprets them under the admitted identity and sends completed location IDs. The journal keeps observations across the appropriate boundaries; the AP server decides the placed reward.

Use the latest automatic gear as the small concrete example. Bit10 indicates its own scene/tutorial milestone; bit11 is the later save-point tutorial. The old code waited for bit11. The correction accepts bit10 while retaining recognition of old completions. It does not grant a Hi-Potion directly: the server happened to place that reward at Gear1 in our test.

Next inspect the permanent WW route bits used for Phantom1/2, and compare a completed route with a temporarily selected route. Then inspect `mines_access.hpp` and `mines_notice.hpp`: Doubleflight ownership controls a specific intact mirror component and a reminder. It must not reopen an already shattered mirror or disable the whole player's controls.

Discussion checks: Why can a wrong check predicate cause a delayed reward even when delivery is healthy? Why is the native gear flag different from the AP location ID? Why does PopTracker need its own access rules?

## Lesson 7: shutdown and support

Read effective `steam_coordinator_v187_candidate.py`, `shutdown_transport_v191_candidate.py`, `progress_overlap_v196_candidate.py`, and `preinstalled_startup.py`. The coordinator waits for the original game process to finish before releasing session resources. It does not automatically close the game. The user-installed mod remains in place.

A missing query reply during observed window destruction can mean normal closure. A missing reply while the original window is still alive remains a fault. A malformed acknowledgement is not silently converted into success. Command11 contains multiple actions, so only the authenticated snapshot actions qualify for this closure handling; mutation commands cannot be treated as harmless reads merely because the opcode matches.

| Symptom | First distinction to establish |
| --- | --- |
| AP is prepared, never attached | Did the installed startup/UE4SS load and provide matching evidence? Get logs before proposing a cause. |
| Native ARM not confirmed | Which stage and native response failed? Do not assume it means Steam was not found. |
| Unknown save association | Is this the correct seed/profile and a recognized snapshot? Do not adopt or delete it to bypass the guard. |
| Check delayed, item absent | Was the location reported yet? If not, investigate observation; if yes, inspect received history and delivery. |
| Popup unseen after boss | Distinguish missing award from UI queued or hidden by a scene. |
| Error only when closing | Inspect command/action, original process/window state and cleanup; don't retry an uncertain operation. |

Finish by having the maintainer explain one ordinary launch, one receipt, one save/reload, and one failure in their own words. Record questions and the actual files reviewed. We can revisit difficult sections without pretending the whole native codebase is understood after a single overview.

## Cleanup requested during walkthrough (2026-11-09)

The 2026-11-10 formatting, naming, comment and source-map pass is recorded in
`review-cleanup-2026-11-10.md`. The list below preserves the requested scope;
substantial hierarchy/ABI redesign remains separate work, not an implied completed
change. See `SOURCE-MAP.md` for the five renamed delivery modules. Older lesson
references retain their original names to make the review history traceable.

- Replace historical feature/version names with names describing present responsibilities. For example, the general receipt planner and delivery supervisor should not be named after Mickey. Review `_candidate` suffixes and stale experimental comments as well.
- Review formatting across maintained first-party source, not just files flagged individually during this walkthrough. Reformat compressed code for human review: consistent indentation and spacing, one statement per line, readable multiline conditions, clearer local names where helpful, and short comments separating meaningful steps. Specifically include native/UE4SS bridge, Mines, boss-drop and popup helpers, and check Python/Lua source for similar compression. Exclude third-party/vendor code and generated outputs from hand-formatting; improve generators if generated readability needs attention. Keep mechanical formatting separate from renaming/refactoring and behavior changes so equivalence can be checked; preserve meaningful indentation, macros, literals and binary layouts. This is pending cleanup, not a completed change.
- Correct the opening comment in `native/transport/common/menu_local_return_v235.h`: it says full-gear Castle support is pending, although the handler implements it. Also review the development-only/lease descriptions in the Mickey journey and catch-up helpers against their production mode-2 branches. These are comment corrections, not requests to change working behavior.
- Add short purpose comments or docstrings where a function's role is not obvious, especially at reporting, delivery, save/load and native-hook entry points. Explain significant inputs, outcomes or side effects only where useful; avoid narrating each line or documenting trivial helpers. Explain non-obvious constants and why safety checks exist. Correct stale comments rather than adding contradictory explanations. Requested during the checks/goal walkthrough on 2026-11-10.
- Organize delivery around a clear entry point, shared transaction/receipt handling, and item-category adapters for consumables, wardrobe, Zodiacs and maps. Preserve the distinction between award confirmation, live receipt membership, save persistence and popup bookkeeping.
- Review accumulated wrapper/inheritance layers and duplicated implementations. Consolidate only where equivalent behavior is established; do not remove a safety check merely because it appears in more than one layer.
- Make the effective packaged source easy to identify, including UE4SS overrides. Update imports, build manifests, source maps, tests and walkthrough links together with any moves or renames.
- Before implementation, preserve a verified recoverable copy of the accepted source, package and applicable installed configuration. Make structural changes separately from behavior fixes, and retain the accepted build until replacement verification passes. Use focused offline checks and require live regression evidence when runtime equivalence cannot be established offline.

This cleanup is requested for release preparation. Scope substantial reorganizations explicitly rather than silently turning naming polish into a native-system rewrite. No runtime files, saves or installed packages were changed when recording this list.

## Review record

The maintainer read linked source during the conversation, asked questions, corrected requirements, and confirmed understanding before moving to subsequent sections. Explanations and source inspection were assisted by Codex. This records a human walkthrough of the paths below, not an independent expert audit, a line-by-line review of every file, or a promise that external community standards have been met. Earlier live gameplay evidence remains separate.

| Reviewed path | Source anchors and discussion |
| --- | --- |
| Launcher and discovery | `worlds/kh02/launcher.py`, effective UE4SS client entry, `steam_discovery.py`: extraction, hashes, module loading, Steam metadata, process identity, and client logging boundaries. |
| Save isolation and startup | Effective `steam_launch_v187_candidate.py`, `preinstalled_startup.py`, profile/preparation/association modules, `title_container.py`, save decoder, readiness reader, native ARM validation: seed identity, fresh versus unknown containers, authorization and acknowledgement. |
| Received items and notices | General planner/delivery modules historically named `mickey_*`, area-map transport, mixed native dispatcher, receipt owner/grant/admission: reserve before execution, inventory verification, uncertain results, and popup/receipt/save distinctions. |
| Save, load, Retry and Continue | Native reusable save pipeline, session publication/lookup, disk-load/branch, checkpoint receipts and lifecycle: exact snapshot associations, branch reconciliation, checkpoint restoration and stale-object invalidation. |
| Abilities, magic and stats | Action/magic ownership, progressive magic, menu greying, situation availability, stat projection and resource carry: native base versus AP increments, current versus maximum resources, no unexplained free healing. |
| Checks and goals | Objective/chest/progress reporting, native observations, goal witness/proof, access requirements, mirror gate and Tide choice: observed completion versus AP reporting, native victory versus unlock, server-controlled release. |
| Travel, restarts and focused retries | Menu visibility/order, world/Forest preparation, focused retry dispatcher and ordinary return helpers: discovery and completion preservation, shared story value, world-specific reconstruction and rollback. Completed-Forest/Ivy-Rail interaction remains a requested live check. |
| Mickey, traps and DeathLink | Companion control/journey/catch-up, trap journal/native handler, DeathLink client/native observation: ownership, actor lifetime, temporary effects, at-most-once attempts and echo suppression. |
| Generation and tracker access | `world.py`, `access_v261.py`, shared JSON, wardrobe selection, tracker `access.lua`: reachability versus item placement, conservative wardrobe logic, tracker green/yellow alternatives. Maintainer corrected Cure to a generation requirement for Zodiac Phantom. |
| UE4SS and Mines | Bridge dispatch, registration policy, weak/object identity, Mines access and notice helpers: engine calls versus permission, consumed attempts, specific interaction component and intact-mirror checks. Maintainer requested general formatting cleanup because compressed code impeded reading. |
| Difficulty, XP and local drops | Difficulty preference/session/pending modules; XP policy/hook; local drop admission/delivery; custom popup service: next-load application, earned-XP scaling, local rewards versus AP items, cinematic deferral and native queue ordering. |
| Tracker lifecycle | `state.lua`, `init.lua`: clear/history rebuild, duplicate indices/checks, bounded state, one-time handlers, layout changes and protected bulk updates. |
| Shutdown and support | Effective coordinator and preinstalled startup: revoke authority, wait for normal game exit, preserve user-installed mod and saves, retain diagnostics. Support recap distinguished preparation, attachment, save association, check reporting, delivery and notification failures. |

Scope limits: not every helper, test, assembly routine, engine offset, or third-party dependency was reviewed individually. Formatting and requirement changes made afterward require their own offline verification. No live test was armed during this walkthrough. Pending cleanup and the two requested live checks are tracked in `steam-ue4ss-rc2-review.md` and the subsequent cleanup report.
