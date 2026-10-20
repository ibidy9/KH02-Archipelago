# Kingdom Hearts 0.2 Archipelago

An Archipelago multiworld randomizer for **Kingdom Hearts 0.2 Birth by Sleep – A Fragmentary Passage**, included in the Steam version of Kingdom Hearts HD 2.8 Final Chapter Prologue.

Find abilities, magic, world access, wardrobe pieces, and other rewards throughout your game—or receive them from friends playing other games in the same multiworld.

**Public beta · Windows/Steam only.** Epic Games Store support is planned separately. A full seeded playthrough and focused feature tests have passed; bug reports from public testing are welcome.

## Features

- **Randomized rewards:** treasure chests, objectives, Castle gears, boss encounters, world exits, Lingering Memories, and level checks from 51 through 70.
- **Ability unlocks:** Cartwheel, Barrier, Counter Blast, Aerial Recovery, Doubleflight, Air Slide, Shotlock, and Spellweaver.
- **Progressive magic:** unlock Firaga, Blizzaga, and Thundaga, then their corresponding Grand Magic. Curaga is a separate unlock.
- **Independent world access:** find access to Castle Town, The World Within, Forest of Thorns, and Depths of Darkness. Choose a starting world or randomize it.
- **Starting loadout options:** choose which abilities and spells you begin with. Random starts only include Depths if you begin with Cartwheel or Barrier.
- **Two goals:** defeat Demon Tide or Zodiac Phantom at the end of the boss rush. Choose Wayfinders, Zodiac relics, or complete wardrobe sets as the goal-unlock requirement.
- **Wardrobe randomization:** all 51 unlockable pieces are included. Objectives are checks independent of their original wardrobe rewards.
- **Wardrobe-set goal:** collect one complete three-piece set in each category: head, arms, back, and pattern.
- **Randomized HP and MP bonuses**, maps, Zodiac relics, and consumables.
- **Clear-data content from the start:** extra chests, objectives, and Lingering Memories are available without a completed vanilla save.
- **In-game AP notifications** for received items and items found for other players.
- **Optional Mickey randomization:** keep his normal behavior, unlock him for Depths, or unlock him for all worlds.
- **XP multiplier, repeatable boss consumable rewards, optional Stun and MP Recharge traps, and DeathLink.**
- **Critical available from the start**, with client commands to change difficulty during a run.
- **Separate AP saves for each seed**, world restarts, and retries for Castle's Demon Tower, Phantom Aqua 3, the Forest Ivy Rail, and the final Forest Darkside.
- **PopTracker support:** an overview, world maps and checks, objectives, and wardrobe sets.

The YAML also lets you prevent important items from being placed behind Zodiac Phantom when that difficult fight is optional.

## Downloads and requirements

Download these files from this repository's **Releases** page:

| File | Purpose |
| --- | --- |
| `kh02.apworld` | World generation, the KH client, and its required mod |
| `kh02-poptracker-0.4.11.zip` | Optional PopTracker pack |
| `START-HERE.txt` | Offline setup instructions |

You need the **Windows Steam version of KH 2.8**, [Archipelago](https://github.com/ArchipelagoMW/Archipelago/releases), and optionally [PopTracker](https://github.com/black-sliver/PopTracker/releases). The installed client was tested with Archipelago **0.6.7**. This world does not require 0.6.8.

You do **not** need to download a separate client executable or install a mod manually. The APWorld includes both. GitHub's automatic **Source code** downloads are not the player installation package.

## Installation and seed generation

1. In Archipelago Launcher, choose **Install APWorld** and select `kh02.apworld`. Restart the launcher afterward.
2. Use **Options Creator** to create a YAML for **Kingdom Hearts 0.2 Birth by Sleep**. Choose your player name, starting loadout, goal, and other settings.
3. Give that YAML to the person generating your multiworld, or use it to generate your own. **The generator also needs this APWorld installed.**
4. Host the generated seed and keep the server address, port, and your exact player name.
5. For PopTracker, put its ZIP in your PopTracker `packs` folder without extracting it, then select the KH 0.2 pack.

## Starting a session

1. Open **Steam** and sign into the account you use for KH. Keep **0.2 closed**.
2. Open **KH 0.2 Client** from Archipelago Launcher.
3. Connect using your server address and player name.
4. Wait for **“AP is prepared.”** The client automatically detects the supported Steam installation and prepares the mod and isolated saves.
5. Launch **Kingdom Hearts HD 2.8 Final Chapter Prologue** through Steam, then select **0.2**.
6. Once the client says it is attached, start a **New Game** for a new seed or load one of that seed's AP saves.
7. If using PopTracker, connect it to the same room and player.

Keep the client open while playing. You do not need to enter a Steam ID or locate the 0.2 executable yourself.

## Saves and ending a session

AP uses its own seed-specific saves. Existing vanilla saves are not used for an AP run. If you see your vanilla saves, stop before loading one and check that the client was prepared and attached.

When finished, **close the game normally, then close the client**. It restores normal startup automatically; you do not need to remove the mod afterward.

If you close the client first, its window stays open while it waits for the game to close. Close the game normally and let cleanup finish. If cleanup reports an error, keep the message and recovery files for troubleshooting.

**Open a fresh KH client window for each new game session.** Reconnecting an old window does not prepare another game launch.

## Changing difficulty

Enter one of these commands in the KH client:

```text
/difficulty beginner
/difficulty standard
/difficulty proud
/difficulty critical
```

Wait for confirmation, then **reload your AP save**. Difficulty changes apply on loading, not immediately during gameplay. Use `/difficulty status` to check your selection and `/help` for available commands.

## Known limitations

- Demon Tide is only accessible when Demon Tide is the selected goal.
- Objective 51 is not available as an AP check and cannot be completed in this randomizer.
- Epic Games Store is not supported by this build.
- Mickey's navigation outside Depths can be imperfect. He can warp to catch up, but may remain outside some boss barriers while still assisting.
- Very long item or player names can extend beyond the popup background.
- Tracker map positions are guides; please report any remaining misplaced markers.

## Reporting issues

Open an issue on this repository with your settings, world and location, what happened, and what you expected. For travel or loading problems, include the save slot, destination, and steps leading up to the issue. Include relevant client errors or logs when available; do not post server passwords.

## Credits

Developed with assistance from OpenAI Codex, with extensive manual in-game testing.

Built for [Archipelago](https://archipelago.gg/) and [PopTracker](https://github.com/black-sliver/PopTracker). Wardrobe and Wayfinder reference artwork was sourced from [KHWiki](https://www.khwiki.com/). Kingdom Hearts and its game assets belong to their respective owners. This is an unofficial fan project.

Magic and map icons in PopTracker 0.4.12 are sourced from [Kingdom Hearts Re:Collection](https://televo.github.io/kingdom-hearts-recollection/), maintained by Televo. Individual asset sources are recorded in the pack; underlying game artwork remains the property of its respective owners.

## License

Original project code is available under the [MIT License](LICENSE). Third-party code retains its own license. Kingdom Hearts artwork, names, and other game assets are not covered by this project's MIT license and remain the property of their respective owners.

## Source and contributions

See [BUILDING.md](docs/BUILDING.md) for source layout and build instructions, [CONTRIBUTING.md](CONTRIBUTING.md) for development guidelines, and [third-party notices](THIRD_PARTY_NOTICES.md) for attribution.
