# Steam UE4SS RC2: acceptance and review boundary

RC2 consolidates the passed manual-mod UE4SS candidate. It is ready for the maintainer walkthrough, not a declaration that a human review is complete. No upload, release edit, installation or new live test is part of this consolidation.

## Included fixes

- Mines access follows received Doubleflight; locked access gives a native reminder without trapping the player. Receiving Doubleflight restores entry; saved reload retained ownership in the earlier access test.
- Phantom1 and Phantom2 checks use permanent completed-route evidence, without waiting for a Mines interaction.
- Automatic Castle gear1 checks at its own tutorial/scene milestone rather than waiting for the save-point tutorial. The previous completion bit remains accepted for old saves.
- Shutdown recognizes a vanished original game window around authenticated snapshot queries, retains strict errors for live-window missing/malformed acknowledgements, and never blindly repeats an uncertain command.
- The effective UE4SS coordinator includes detailed failed-query reporting.
- Standard PopTracker0.4.13 includes Doubleflight access for WW chests12/13. No personal pack is shipped.

## Evidence

| Observation | Evidence | Limit |
| --- | --- | --- |
| Mines prompt/access/reminder | `mines-message-passed-2026-11-05.md` and the capture it identifies | Focused WW test, not every mirror |
| Both Phantoms report before Mines | `phantom-reporting-passed-shutdown-followup-2026-11-05.md`; workspace `research/phantom-reporting-live-20261106T013044Z` | Shutdown failed in that older candidate |
| Automatic gear1 before save point; normal shutdown | `gear-shutdown-passed-2026-11-05.md`; workspace `research/gear-shutdown-live-20261106T022556Z` | Exact closing query action was not logged; one clean close is not every race |
| Other UE4SS receipt/lifecycle/manual-install paths | Existing integration status and passed-test documents | Earlier focused coverage remains relevant; not all gameplay code was rewritten |

The maintainer's recent seeded playthrough was partial. It found the issues addressed here; it was not completed. The older implementation had a completed playthrough. Keep those claims separate.

## Binary and source boundary

Accepted input hashes are in `tools/ue4ss-rc2-inputs.json`. Consolidation copies the exact passed APWorld, not a rebuild. The mod archive changes only `KH02-AP-INSTALL.txt` to name tracker0.4.13. Native binaries, Lua code, config, paks, all eight runtime choices, and the generator remain unchanged from the passed input. The effective shared-plus-UE4SS Python sources match that input exactly.

The earlier `package-final` gear build had a Windows cp1252 Python-source error caught before game launch. `package-encoding-fixed` passed the live session. All packaged Python files are compiled from bytes during consolidation so this failure is checked independently of imports exercised by unit tests.

No experiment installers, local room servers, test saves, seed files, logs or personal tracker archives are included in player delivery. Historical experiments are retained in the workspace for recovery; removing them is not required to ship a clean archive.

## Remaining work

Update 2026-11-10: the guided main-path walkthrough is recorded in
`CODE-WALKTHROUGH.md`. Formatting/naming cleanup and the Cure correction are
implemented and checked offline in a separate private candidate; see
`review-cleanup-2026-11-10.md`. RC2's accepted delivery remains unchanged. The Wayfinder mirror check has passed; the Forest check remains pending and unarmed. Prepared instructions are in
`review-live-checks-ready-2026-11-10.md`.

### Required pre-release logic fix: Cure before Zodiac Phantom

Requested 2026-11-10 during the AP world walkthrough. Implemented in the separate cleanup candidate with generation/Lua verification; the original consolidated RC2 candidate is unchanged.

- Require Cartwheel, Barrier, and Cure Magic in generation access to Zodiac Phantom, including its round-5 check, dependent objective/checks, and the boss-rush goal completion rule. Do not apply this combat requirement to the earlier rush rounds or mirror entry.
- A restriction forbidding Cure at Phantom's own rewards is insufficient: generation must also prevent indirect dependencies such as Phantom awarding world access needed to reach Cure. Retain the existing placement restriction as appropriate, but make access itself require Cure.
- Preserve PopTracker behavior: green only with fight access and all three abilities; yellow with fight access but any of the three missing; red without fight access.
- Update the shared rule catalog, exported tracker rules, explanatory notes, and release artifacts together. Verify offline that each missing ability blocks generation access, all three permit it when other requirements hold, dependent checks/goal use the same requirement, and tracker yellow remains available. Preserve the accepted package before implementing; this request does not arm a live test or change existing seeds.

### Requested pre-release live check: Wayfinder-only boss-rush access

Passed 2026-11-10 in the isolated cleanup-candidate session. See `wayfinder-mirror-passed-2026-11-10.md` for evidence and limits. Original test requirements follow.

- Use an isolated test profile, preserving development and playthrough saves. Select Zodiac Phantom (`boss_rush`) as the goal and Wayfinders as its unlock requirement, with WW accessible.
- Prepare a verified state with no Zodiac relics and no already-spawned boss-rush mirror/completed restoration scene. Prefer a reusable WW save with ordinary world progression complete to avoid unrelated Phantom-route behavior.
- In one session, verify the mirror remains locked with only two distinct Wayfinders, then deliver the third through the AP delivery path.
- Approach the normal center trigger and confirm the restoration scene plays, the mirror appears and permits boss-rush entry without awarding any Zodiacs. Capture ownership and access evidence alongside the user's observation. Lighting all Zodiac floor symbols is not a requirement for this test.
- No boss-rush completion is required to establish this access result. Finish offline preparation before requesting participation; arm only when the user says they are ready.

Walk through the actual source with the maintainer, concentrating on installation/file access, seed/save ownership, acknowledgement and replay safety, lifecycle transitions, and troubleshooting. Keep a record of what the maintainer has actually reviewed. These notes and assistant checks are preparation, not independent human review or a promise of moderation approval.

### Requested pre-release live check: completed Forest followed by Ivy Rail retry

Same-Forest completed-run regression was found, corrected and live-passed on 2026-11-10 local time. See `forest-rail-story-integrated-2026-11-11.md`. Cross-world retry scope remains unchanged/unresolved. Original requested checks follow.

- Complete Forest first and establish the completed state, including Path's End discovery and the final Darkside completion flag.
- Select Retry Forest Ivy Rail, complete the rail encounter, then play forward through the rest of Forest normally. Do not substitute teleportation for this requested traversal.
- Record story/completion/discovery state at the relevant boundaries. Observe whether the final Darkside respawns, whether required triggers and exits function, and whether Path's End and its save point behave correctly. Check for repeated AP location rewards or lost previously earned destinations.
- Reason for testing: the rail retry sets story 321 but preserves the final Darkside completion and discovery bits. Ordinary within-Forest save travel preserves story values 280–350 rather than reconstructing the latest earned milestone. Code inspection alone does not establish how the encounter scripts handle this combination; this is an unresolved interaction, not a confirmed bug.
- Prepare suitable combat abilities/stats to minimize replay burden. Do not require a second run merely to observe the same sequence. Finish offline preparation before requesting participation and arm only when the user is ready.

Readable save-profile management beyond the existing profile labels/instructions remains a separate UX topic. Epic support remains deferred. Do not silently add either to this candidate.
