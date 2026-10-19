# Kingdom Hearts 0.2 PopTracker 0.4.11

An optional tracker for the KH 0.2 Archipelago world. Put the ZIP in PopTracker's `packs` folder and connect to the same Archipelago room and player as the game client.

The pack includes an ability/world overview, world check lists and maps, objectives, level checks, and wardrobe pieces arranged into matching sets. Wayfinder and Mickey displays follow the seed's options. Accessibility colors follow the world rules; yellow marks the documented difficult alternatives. Tracker markers are guides and can be corrected without changing item or location IDs.

Artwork attribution and generation prompts are recorded in the source manifests. Kingdom Hearts artwork belongs to its respective owners. Original ability, spell, and parchment-map illustrations were made with AI image assistance. Developer-local file paths have been removed from the manifests; image files and gameplay logic are unchanged from the tested 0.4.11 pack.

The pack uses bounded state tables and registers its AP callbacks once. It does not use polling timers, animated image playback, or growing event histories.
