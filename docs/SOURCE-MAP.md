# Maintained source map

The launcher loads the bundled Python client. Packaging takes shared modules from
`payload/client/kh02`, then replaces same-named modules with those in
`integrations/ue4ss/client`. The packaging verification report records the exact
effective source for each file. An overridden shared module is not an additional
client running alongside the override.

## Receipt and delivery responsibilities

| Current module | Former name | Responsibility |
| --- | --- | --- |
| `receipt_policy.py` | `mickey_receipts_v171.py` | Interpret received history and plan missing save receipts. |
| `item_delivery.py` | `mickey_delivery_v171.py` | Serialize policy/native delivery and resolve acknowledgements. |
| `notification_delivery.py` | `mickey_notice_delivery_v171.py` | Coordinate popup submission separately from item membership. |
| `notification_queue.py` | `mickey_notifications_v171.py` | Persist seed-scoped notification attempts. |
| `shared_client.py` | `mickey_shared_client_v171.py` | Coordinate checks, goals, delivery and notifications for one admitted owner. |

The remaining Mickey-specific modules configure or service the companion. Some
historical versioned subclasses still compose the active client. Their filenames
are implementation identifiers, not separate programs or a declaration that the
features are untested. They remain where removing them would require restructuring
the tested call chain. The `StatDelivery` class currently also handles non-stat
policy items; this is a retained internal class name, not its complete scope.

## Engine and native code

- `integrations/ue4ss/native`: validated UE4SS lookup, reflected calls, game-thread
  pump and scoped Mines interaction. It does not own AP receipt decisions.
- `native/transport/common`: save/receipt lifecycle, gameplay gates, travel,
  local rewards and native notification handling. `.inc` files are textual C
  includes and can depend on their containing translation unit.
- `native/transport/variants`: starting-world/fresh-or-resume variants.
- `integrations/ue4ss/core-bridge-review.patch`: explicit engine-call substitutions
  applied when assembling the UE4SS transport from the shared native sources.
- `native/startup` and `integrations/ue4ss/startup`: early save redirection and
  supported-build startup behavior. Export names and binary interfaces are kept
  stable during readability cleanup.

The source manifests pin exact assembly inputs. Do not update them to make a
failure disappear: compare the change with the preserved baseline and verify it.
Formatting does not require replacing installed live-proven binaries; the cleanup
report records token equivalence and separate compilation results.

## Generation and tracker

`worlds/kh02/world.py` creates the AP graph, pool and slot data. The JSON catalog in
`worlds/kh02/data/access-rules-v261.json` owns location access requirements. Run
`python tools/export_tracker_rules.py` to regenerate the readable tracker copy,
or add `--check` to verify it. `poptracker/kh02-ap/scripts/access.lua` evaluates
the exported rules for display. Green/yellow recommendations may intentionally
differ from generation requirements.

## Boundaries

Formatting and naming cleanup do not justify removing receipt, ownership, or
uncertainty checks. Further hierarchy/ABI changes are separate refactors needing
their own evidence. Third-party code, generated binary layouts, saves and private
test captures do not belong in cosmetic cleanup.
