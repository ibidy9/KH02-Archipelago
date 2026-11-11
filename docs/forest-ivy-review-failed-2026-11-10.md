# Completed Forest Ivy Rail retry: behavior failed

Closed capture: research/forest-ivy-review-live-20261111. Seed 66144286956750314311, player ReviewForestRetr. Normal cleanup confirmed. Server history and isolated profile preserved; no save modified by investigation.

User completed Forest, saved slot 1 and activated ending, then saved again as instructed before retry. Ivy Rail replay and landing encounter worked, Rocky Path save appeared and Paths End remained listed. Walking forward respawned final Darkside. User defeated it and activated ending again; returned with control. No duplicate boss or exit AP rewards; earlier checks retained. These safety observations do NOT make the requested behavior pass.

Desired behavior: replay Ivy Rail without resetting downstream completed progression. User proposes retaining story and routing to rail only. This is a candidate hypothesis, not yet tested.

Static finding: native/transport/common/menu_forest_v149.inc selects mode 1 for dw_08_Lv_Autosave_06. menu_forest_v149.h mode 1 changes both story copies to 321 and preserves switches/discovery. That explains the observed rollback. Historical docs/current-handoff.md Forest rail capture records story 321 rail, 322 landing, 323 Rocky Path, and warns position-only warp does not reconstruct the one-shot challenge. Existing route uses native map reload/spawn, not a position-only warp, so preserving story on this route remains worth checking; no proof yet that the challenge rearms with completed story.

No source/native installation change made for a fix. Next offline work: audit exact Autosave_06 loading and encounter activation, prepare smallest isolated candidate retaining story; verify focused retry modes remain unchanged. Back up source/binaries/config before editing. Do not mark this fixed based solely on offline tests or erase historic story321 requirements without evidence. No next test armed.
