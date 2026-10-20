# Third-party notices

The root MIT license covers original project code. It does not relicense third-party code or game assets.

- **Archipelago:** integration APIs and Archipelago logo. MIT license retained in `licenses/Archipelago-MIT.txt`. The in-game AP pickup icon is derived from Archipelago's `data/icon.png`.
- **Python and SQLite fallback:** the release includes Python 3.13 SQLite support for the frozen Archipelago client. Its Python redistribution notice is retained in `licenses/Python-LICENSE.txt` and in the imported payload.
- **PopTracker:** the tracker pack targets PopTracker; the PopTracker application itself is distributed separately.
- **Kingdom Hearts artwork and assets:** maps, characters, logos, item artwork, and game-derived PAK contents belong to their respective owners, including Disney and Square Enix. No ownership of those assets is claimed.
- **KHWiki:** wardrobe, Wayfinder, and chest reference artwork sources are recorded in the tracker asset manifests, including their original URLs where available. Underlying game artwork retains its original ownership.
- **Kingdom Hearts Re:Collection (Televo):** the magic and map icons in tracker 0.4.12 are sourced from this collection. Exact files, source URLs, and hashes are recorded in `poptracker/kh02-ap/recollection-sources.json`. Credit for the collection does not imply ownership of all underlying game artwork.
- **Additional ability icons:** Air Slide comes from Re:Collection, Doubleflight from SilentBrow's BBSPopTracker, and the 0.2 Spellweaver logo from KHWiki. Exact source URLs are in `poptracker/kh02-ap/ability-replacement-sources.json`. Attribution does not imply ownership or a blanket license for underlying game artwork.
- **HD ability symbols:** Cartwheel, Barrier, Counter Blast, and Aerial Recovery use repurposed Dream Drop Distance HD symbols. Shotlock uses a Birth by Sleep HD targeting graphic. Extracted from installed game assets using OpenKH; underlying artwork belongs to Disney / Square Enix. Exact asset paths are recorded in `poptracker/kh02-ap/ability-replacement-sources.json`.
- **Generated/edited tracker artwork:** the native HP lettering was recoloured green with AI assistance. All eight displayed Aqua ability illustrations have been replaced with game artwork in 0.4.12. Original ability files remain for provenance; the separate personal pack is not a public release asset.

Zig, Archipelago, and game-asset tooling are separate development dependencies. Their executables are not included in this source repository.
