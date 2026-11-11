# Forest discovery/menu investigation

Cross-world Ivy Rail test passed; closed evidence research/forest-rail-crossworld-passed-20261111. Rail replayed, no repeated landing or final Darkside fight, exit worked, AP history unchanged, clean shutdown. Separate UI issue: WW lists Travel to Forest instead of Uncertain Path and omits Restart Forest; later saves and retries remain present.

Read-only original pretest decoded slot1 (build/forest-rail-preserve-story-20261111/decoded/decoded.raw) has story360 and discovery word0x18 at serialized offset0x3320. This predates both preservation tests. It is consistent with Rocky/End discovered and first Forest save missing; exact resolver index should be verified against catalog before hardcoding any bit (production already resolves dynamically). No evidence that cross-world retry deleted discovery. Fresh Forest start + walking through without activating the first save is a plausible cause, not yet proved at the initial trigger.

menu_visible151 uses only the first save's resolved discovery bit for Travel, ordinary first-save and Restart visibility. Ordinary first row also requires native visibility. menu_return215 independently refuses selection of an undiscovered destination. Therefore merely replacing Travel label or forcing first-row visibility would be incomplete: return admission must agree.

Proposed narrow fix: for Forest, later discovered saves prove the world has been visited and first save is reachable. Use the same resolved Forest discovery predicate for first-save/Travel/Restart visibility and first-save return admission, without manufacturing saved discovery bits. Keep Rocky/End individual gating and true restart clearing unchanged. Existing restarts preserve first bit; fallback must not expose later saves after a true restart. Do not apply assumptions to other worlds. Needs offline mask matrix and one combined WW-to-Uncertain return/Restart list check.

No menu code, profile or installed file changed during this investigation. No new test armed.
