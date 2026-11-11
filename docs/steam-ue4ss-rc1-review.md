# Steam UE4SS release candidate 1 — review boundary

The reference is the Computer 1 Forest chest build. RC1 ships the same game binaries, Lua scripts, asset paks, AP world logic and PopTracker 0.4.12. The packaged coordinator changes how ordinary waits are reported. The mod archive changes its player instructions. The packaging filter keeps only runtime DLLs selected by the catalog.

Start the code walkthrough at integrations/ue4ss/Scripts/main.lua. The ClientRestart hook invalidates the old player context and observes the new controller and pawn. An observation does not authorize an award. The native host validates the live objects and calls the reflected award functions where those calls were proven. The client still owns AP check reporting, seed identity, and durable item receipts. Crossing a room boundary can replace Aqua's pawn without making old AP items new again.

Keep the native startup path for selecting isolated storage before the title screen reads saves. Keep the eight world/mode transport choices for the exact supported Steam build. Story travel, retries, gating and save reconciliation depend on game-specific behavior. The Forest capture supports the chest-to-server-to-receipt path. It is not evidence of a fresh playthrough across all of those features on RC1. No additional engine-hook conversion is required for this candidate.

The lab installers, room peers, fixtures and rollback tools stay outside the player archives. Build and audit details are in integrations/ue4ss/BUILDING.md. The host/startup artifacts are the accepted binaries; matching imports or exports alone does not prove a byte-identical rebuild. RC1 remains private pending a player-install playthrough and maintainer review.
