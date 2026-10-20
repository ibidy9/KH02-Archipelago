# Kingdom Hearts 0.2 PopTracker 0.4.12

An optional tracker for the KH 0.2 Archipelago world. Put the ZIP in PopTracker's `packs` folder and connect to the same Archipelago room and player as the game client.

The pack includes an ability/world overview, world check lists and maps, objectives, level checks, and wardrobe pieces arranged into matching sets. Wayfinder and Mickey displays follow the seed's options. Accessibility colors follow the world rules; yellow marks the documented difficult alternatives. Tracker markers are guides and can be corrected without changing item or location IDs.

Magic and map icons are sourced from [Kingdom Hearts Re:Collection](https://televo.github.io/kingdom-hearts-recollection/), compiled and maintained by Televo. Exact files, source URLs, and hashes are recorded in `recollection-sources.json`. Underlying Kingdom Hearts artwork remains the property of its respective owners.

The Aqua ability illustrations have been replaced with game artwork; and the HP lettering retains its AI-assisted green recolour. Their provenance is recorded separately. Other artwork sources, including KHWiki and game-extracted assets, remain recorded in the existing manifests.

Version 0.4.12 replaces magic/map artwork and expands and centres the overview. Item IDs, AP callbacks, accessibility rules, and world maps are unchanged. Wardrobe sets now run vertically in every category, with space between Head, Arms, and Back.

The pack uses bounded state tables and registers its AP callbacks once. It does not use polling timers, animated image playback, or growing event histories.

Air Slide is from Televo’s Re:Collection; Doubleflight is sourced from SilentBrow’s BBS PopTracker; the Spellweaver logo is 0.2 artwork hosted by KHWiki. See `ability-replacement-sources.json` for individual sources.

Cartwheel, Barrier, Counter Blast, and Aerial Recovery use symbols extracted from Dream Drop Distance HD. Shotlock uses a targeting graphic extracted from Birth by Sleep HD. These are repurposed game symbols, credited to Disney / Square Enix; extraction used OpenKH. See `ability-replacement-sources.json`.
