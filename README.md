# Kingdom Hearts 0.2 Archipelago Randomizer

An Archipelago multiworld randomizer for **Kingdom Hearts 0.2 Birth by Sleep – A Fragmentary Passage**, included in the Steam version of Kingdom Hearts HD 2.8 Final Chapter Prologue.

Find abilities, magic, world access, wardrobe pieces, and other rewards throughout your game—or receive them from friends playing other games in the same multiworld.

**Steam public beta · Windows/Steam only.** Epic Games Store support is planned separately. The earlier build passed a full seeded playthrough. This UE4SS update has partial playthrough and focused regression coverage, including the final Wayfinder, Forest retry, and save-point access checks. The maintainer has reviewed the main implementation paths. It remains a public beta; please report problems.

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
| `kh02.apworld` | World generation and the KH client |
| `kh02-steam-mod.zip` | Manually installed game mod |
| `kh02-poptracker-0.4.13.zip` | Optional PopTracker pack |
| `PLAYER-INSTALL.txt` | Offline setup instructions |

You need the **Windows Steam version of KH 2.8**, [Archipelago](https://github.com/ArchipelagoMW/Archipelago/releases), and optionally [PopTracker](https://github.com/black-sliver/PopTracker/releases). The client has been tested with Archipelago **0.6.7 and 0.6.8**. This world does not require 0.6.8.

The APWorld includes the client. Install the game mod separately using the steps below. GitHub's automatic **Source code** downloads are not the player installation package. These instructions describe the UE4SS candidate; older public builds used a different installation method.

## Installation and seed generation

1. In Archipelago Launcher, choose **Install APWorld** and select `kh02.apworld`. Restart the launcher afterward.
2. Use **Options Creator** to create a YAML for **Kingdom Hearts 0.2 Birth by Sleep**. Choose your player name, starting loadout, goal, and other settings.
3. Give that YAML to the person generating your multiworld, or use it to generate your own. **The generator also needs this APWorld installed.**
4. Host the generated seed and keep the server address, port, and your exact player name.
5. For PopTracker, put its ZIP in your PopTracker `packs` folder without extracting it, then select the KH 0.2 pack.

With the game closed, use Steam's **Browse local files**, then open the **KINGDOM HEARTS 0.2 Birth by Sleep** folder inside the collection. Extract `kh02-steam-mod.zip` here, merging its `Binaries` and `Content` folders with the existing ones. If asked to replace an existing file, stop and check which mod owns it. Keep the ZIP as a record of the files you installed.

## Starting a session

1. Open **Steam** and sign into the account you use for KH. Keep **0.2 closed**.
2. Open **KH 0.2 Client** from Archipelago Launcher.
3. Connect using your server address and player name.
4. Wait for **“AP is prepared.”** The client detects the supported Steam installation, checks the installed mod, and prepares isolated saves.
5. Launch **Kingdom Hearts HD 2.8 Final Chapter Prologue** through Steam, then select **0.2**.
6. Once the client says it is attached, start a **New Game** for a new seed or load one of that seed's AP saves.
7. If using PopTracker, connect it to the same room and player.

Keep the client open while playing. You do not need to enter a Steam ID or locate the 0.2 executable yourself.

## Saves and ending a session

AP uses its own seed-specific saves. Existing vanilla saves are not used for an AP run. If you see your vanilla saves, stop before loading one and check that the client was prepared and attached.

When finished, **close the game normally, then close the client**. The mod stays installed for your next AP session. Closing the client alone does **not** disable the mod.

If you close the client first, its window stays open while it waits for the game to close. Close the game normally and let cleanup finish. If cleanup reports an error, keep the message and recovery files for troubleshooting.

**Open a fresh KH client window for each new game session.** Reconnecting an old window does not prepare another game launch.

## Switching between AP and vanilla

To play vanilla, first close the game normally, let the client finish cleanup, and close the client. Open the **KINGDOM HEARTS 0.2 Birth by Sleep** installation folder.

Create a folder **outside the game installation**, such as `KH02 AP disabled` on your Desktop. Make `paks` and `loaders` folders inside it, then move these four files:

| From | File | Move into |
| --- | --- | --- |
| `Content\Paks\~mods` | `KH02_AP_RestartPrototype_P.pak` | Your backup's `paks` folder |
| `Content\Paks\~mods` | `TresGame-WindowsNoEditor_99_P.pak` | Your backup's `paks` folder |
| `Binaries\Win64` | `dinput8.dll` | Your backup's `loaders` folder |
| `Binaries\Win64` | `dwmapi.dll` | Your backup's `loaders` folder |

Launch normally **without the KH client** to use your vanilla saves. Move the files rather than deleting them, and leave all saves, AP profiles, and other files alone. Removing only the paks does not disable the native AP loaders. If another mod shares these loaders, disabling them may disable that mod too.

**To return to AP:** with the game closed, move each file back to its original folder. Don't overwrite an unexpected existing file. Open a fresh KH client, connect to your seed, and wait for **“AP is prepared”** before launching. Your AP saves remain in their seed profile throughout.

## Removing old seed saves

Each seed and player slot has its own profile. Reconnecting to the same seed reuses it; playing new seeds adds more folders. The client never deletes old profiles automatically.

1. Close the game normally, let cleanup finish, then close the KH client.
2. Paste `%LOCALAPPDATA%\KH02AP\profiles` into File Explorer's address bar.
3. Find the seed you no longer want. Folder names start with the seed name, followed by the player slot (`P1`, for example) and an identifier. Open `profile.json` in Notepad and check its `label` for the seed and player name. Don't edit it.
4. Move the **entire seed folder** somewhere else to keep a backup, or delete that folder if you are finished with it.

Keep the folder intact when backing it up: saves and item-receipt history belong together. Don't delete individual files inside a profile to troubleshoot a run you want to keep. Removing a profile removes that seed's local progress, not its server-side checks, and does not affect vanilla saves or other seed profiles. To resume a backed-up profile, restore the whole folder to its original location while the game and client are closed; don't merge it with a newly created profile.

## Changing difficulty

Enter one of these commands in the KH client:

```text
/difficulty beginner
/difficulty standard
/difficulty proud
/difficulty critical
```

Wait for confirmation, then **reload your AP save**. Difficulty changes apply on loading, not immediately during gameplay. Use `/difficulty status` to check your selection and `/help` for available commands.

The Mines entrance requires Doubleflight. Before you receive it, the entrance stays locked and displays a reminder when you approach.

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

OpenAI Codex generated most of the implementation and assisted with investigation, testing tools, and debugging. The maintainer designed the randomizer settings and logic and performed extensive manual in-game testing. The maintainer reviewed the main implementation paths and discussed their behavior, safety checks, and failure handling. The scope and limits are recorded in [the code walkthrough](docs/CODE-WALKTHROUGH.md); this was not a line-by-line audit of every helper or third-party dependency.

Built for [Archipelago](https://archipelago.gg/) and [PopTracker](https://github.com/black-sliver/PopTracker). Wardrobe and Wayfinder reference artwork was sourced from [KHWiki](https://www.khwiki.com/). Kingdom Hearts and its game assets belong to their respective owners. This is an unofficial fan project.

Magic and map icons in PopTracker are sourced from [Kingdom Hearts Re:Collection](https://televo.github.io/kingdom-hearts-recollection/), maintained by Televo. Individual asset sources are recorded in the pack; underlying game artwork remains the property of its respective owners.

## License

Original project code is available under the [MIT License](LICENSE). Third-party code retains its own license. Kingdom Hearts artwork, names, and other game assets are not covered by this project's MIT license and remain the property of their respective owners.

## Source and contributions

For a quick guide to the implementation, see the [code-review cheatsheet](docs/CODE-REVIEW-CHEATSHEET.md).

See [BUILDING.md](docs/BUILDING.md) for source layout and build instructions, [CONTRIBUTING.md](CONTRIBUTING.md) for development guidelines, and [third-party notices](THIRD_PARTY_NOTICES.md) for attribution.
