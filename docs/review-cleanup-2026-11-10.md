# Review cleanup and Cure logic correction

Offline preparation completed 2026-11-10. No game/client was launched, no installed
mod or APWorld was replaced, no save/profile was edited, and no live test was armed.
The original RC2 delivery is retained unchanged.

## Review record

`CODE-WALKTHROUGH.md` now records the main paths examined with the maintainer,
questions and corrections, and the limits of that review. It does not claim every
line, offset or dependency was reviewed or promise moderator approval.

## Behavior-preserving maintenance

- Reformatted maintained native C/C++/include files and Python modules. Verified
  native token equality and Python AST equality before separate comments/import
  naming edits. Includes, string contents, numeric constants, declarations and
  binary layouts were not redesigned. Added formatter configuration.
- Regenerated the strict UE4SS bridge patch against formatted inputs and updated
  source manifests after equivalence checks. All eight variants assembled and
  compiled with the existing strict warning flags.
- Binary comparison found only diagnostic `__LINE__` constants changed in native
  executable sections; all other executable bytes matched the accepted builds.
  Rebuilt binaries are development evidence, not replacements in delivery.
- Corrected known stale Castle-return, Mickey, XP, local-drop, trap, popup and
  host descriptions. Added concise responsibility documentation and a source map.
- Renamed five general `mickey_*` modules to `receipt_policy`, `item_delivery`,
  `notification_delivery`, `notification_queue` and `shared_client`; updated
  imports and packaging. No duplicate compatibility modules are shipped. The
  packaged client AST matches accepted code after normalizing those imports and
  excluding docstrings.
- Made exported tracker rules readable and added a deterministic exporter/check.
  Reviewed tracker state/init formatting and expanded compressed access branches.

Historical ABI/export names, native version suffixes and tested composition
subclasses remain. Flattening the hierarchy or renaming every native symbol would
be a separate refactor, not formatting. No safety checks or wrappers were removed
without equivalent evidence. `SOURCE-MAP.md` identifies effective overrides and
remaining historical names. Third-party code was not reformatted.

## Intentional generation change

Cartwheel, Barrier and Cure Magic are now prerequisites for Zodiac Phantom's
round-5 check, Objective 42 when included, and the boss-rush goal. Earlier rush
rounds and mirror entry do not gain these combat requirements. The placement
restriction on Cure remains; the access rule also prevents indirect dependencies.
PopTracker remains yellow with fight access if any kit item is missing, green
with all three, and red without access. Existing generated seeds are unchanged.

## Offline evidence

Workspace evidence directory: `build/review-cleanup-20261110`.

- Repository tests: 34 tests, 33 passed, one optional SQLite environment test
  skipped; UE4SS Python tests: 15 passed.
- Native bridge, registration policy and actual registration backend tests passed
  with assertions enabled, including reentry, stale identities, failures and
  consumed attempts. All eight native core builds passed.
- 72 actual AP fills across both goals, three requirements, four starting worlds
  and three Mickey modes were beatable and fulfilled accessibility. 576 actual
  graph/Lua evaluations covered every combat-kit subset in those configurations.
- Both isolated room archives were generated using the packaged world. Packaged
  client connected to temporary loopback servers, received three Wayfinders and
  synchronized without duplicates. No game preparation/attachment ran. Temporary
  servers and clients closed at the end. Server-mutated package metadata and AP's
  truncated player name were corrected in the offline harness before final pass.
- New package validates all client, runtime and installed-mod manifests. Native
  binaries/configuration remain the accepted inputs. Personal tracker excluded.

## Recovery and delivery

Verified backup: `backups/before-review-cleanup-20261110T231836Z`, containing a
747-member archive, SHA-256 manifest and restoration instructions. It preserves
the prior source, binaries/configuration inside accepted packages, and review
documents. Installed files were outside the mutation scope and remain untouched.

Private candidate: `build/review-cleanup-20261110/candidate-final/delivery`.
Build using `tools/package_review_cleanup.py` with the pinned RC2 input directory
and a new output directory. Never overwrite the accepted delivery to make a new
candidate. Standard tracker remains 0.4.13 in this private candidate; public
versioning/upload is a later decision. No personal pack is included.

The only remaining requested live checks are Wayfinder-only mirror access and
completed Forest followed by Ivy Rail retry. The next session also exercises the
renamed client import path; no separate Potion-only boot is requested.
