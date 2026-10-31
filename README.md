# Protolaunch

Protolaunch is a slingshot upgrade game being developed in C++ for COMP3016.
Drag an object back, release it to launch, collect coins, and spend them on
upgrades to fly further and earn bigger payouts.

## Gameplay

**Launch → collect coins → earn a payout → upgrade → fly further.**

- Aim and launch the object using a mouse-driven slingshot.
- Collect coins during each attempt.
- Spend coins on upgrades between launches.
- Use those upgrades to improve future attempts and earn larger payouts.
- Unlock air boosts for keyboard control during flight.

## Controls

| Input | Action |
| --- | --- |
| Click and drag the object | Pull back and aim the slingshot |
| Release the mouse button | Launch the object |
| R | End a launched run and show its payout; reset the ball before launch |
| Up arrow during flight | Use an air boost, once unlocked |
| Escape | Pause and open the menu; return from Options or resume a paused run |
| Mouse, arrow keys or Tab | Select a menu button |
| Enter or Space | Activate the selected menu button |

The main menu offers Play, Shop, Environments, Save / Load, Options and Quit. During a run it
also offers Resume and End run. Options controls fullscreen, VSync and anti-aliasing; changes
apply immediately for the current session.

The HUD tracks your furthest horizontal distance from the launcher, current
height above the ground and highest altitude this run, all in metres. Measurements
start on launch, remain when paused and reset with a new run. Run time counts
active play after launch, including rolling, and excludes pauses and menu time.

## Environments

Open **Environments** from the main, pause or results menu. Yard is available at
the start. Unlock each following area by reaching **either** target in the previous
area; milestones are recorded when the run ends, including runs ended with R.
Personal distance and height records belong to each environment, and never decrease.

| Environment | Target in previous area | Gravity (m/s²) | Ground grip | Rolling resistance |
| --- | --- | --- | --- | --- |
| Yard | Available from the start | 9.8 | 0.9 | 0.4 |
| Coast | Yard: 250 m distance OR 60 m height | 8.5 | 0.6 | 0.25 |
| Peaks | Coast: 600 m distance OR 180 m height | 12.0 | 0.3 | 0.08 |
| Storm | Peaks: 1,000 m distance OR 350 m height | 10.8 | 0.8 | 0.3 |
| Orbit | Storm: 1,600 m distance OR 600 m height | 3.2 | 0.45 | 0.12 |
| Void | Orbit: 2,500 m distance OR 1,000 m height | 5.5 | 0.2 | 0.06 |

Each area uses its matching ground, mountain and cloud artwork, plus its own sky
colour. Higher gravity shortens flights; lower grip and rolling resistance make
surfaces more slippery. Upgrades, skins and currency carry across every area.

Unlocked areas stay available and can be revisited freely. Selection applies to
the next new run; Resume continues the paused run in its original environment.
The HUD names the active environment. Automatic and manual saves include records
and the selected area. Older saves retain existing progression and start in Yard.
Tune environments and milestones in `EnvironmentManager.h`.

## Run payouts

Runs finish when the ball rests on the ground for 0.75 seconds, or when you
choose End run or press R during flight. The results screen shows your run coins,
distance, maximum height, run time and final payout, with Play again, Shop and Main menu
buttons. Trampolines award 5, 10 or 25 coins directly to the run counter, once per
platform per run.

**Payout = run coins × distance multiplier × height multiplier × time multiplier.**

Each multiplier starts at ×1: distance adds ×1 per 250 metres (up to ×4), maximum
height adds ×1 per 100 metres (up to ×3), and time adds ×1 per 60 seconds (up to ×2).
Multipliers and their combined total round to two decimal places; the final payout
rounds down to whole coins. Rewards are credited once to your saved coin bank.
Tune these values in `RunRewardSettings`
through `LevelConfig::runRewards`.

## Shop and upgrades

Spend banked coins in the shop from the main menu, pause menu or results screen.
All upgrades have five levels, and prices double after each purchase. Purchases
apply on your next run, so shopping while paused does not change the current run.

| Upgrade | Effect per level | Starting price |
| --- | --- | --- |
| Launcher | +8% launch power and a new launcher appearance | 25 coins |
| Bands | +8 pixels of maximum pull distance | 20 coins |
| Coin Magnet | +20 pixels of coin pickup reach | 15 coins |
| Lucky Loot | Higher relative chances of 2 and 3 value coins | 30 coins |
| Glider | 6% less gravity | 40 coins |
| Air Boost | One extra Up-arrow boost per run | 50 coins |

Air boosts add upward speed while keeping horizontal motion. Charges refill
on a new run. The HUD shows the charges remaining.

The Coin Magnet shows a gold outline around its pickup area, growing with each
upgrade level. Coin pickups produce gold `+1`, `+2` or `+3` labels and sparks;
paid trampoline bounces produce mint `+5`, `+10` or `+25` effects. Feedback rises
and fades, pauses with the game and clears when starting a new run.

Coins and purchased levels save automatically to `progress.dat` in SDL's
Protolaunch preferences folder, outside the build and asset folders. Copy this
file to the corresponding preferences folder to move progression between machines.
Choose **Save / Load** from the main or pause menu to explicitly save or reload
coins, upgrade levels, skins and environments. Feedback confirms success or explains a failure;
missing or invalid saves leave your current progress intact. These buttons use
the same plain-text `progress.dat` file as automatic saving. Runs and options are
not saved. Loading while paused keeps the current run and its upgrade effects;
loaded upgrade levels apply on the next run, while the equipped skin updates immediately.
Prices, limits and effects are defined in `UpgradeManager.h` / `.cpp`.

## Skin shop

Choose **Skin shop** from the upgrade shop to browse 40 standard skins and 24
ultra-premium skins. Switch collections with the Standard / Ultra premium tabs
and use Previous / Next to browse each collection. The cream starter and original
blue Diamond skin are unlocked from the start.

Buy a skin with banked coins to unlock and equip it. Owned skins can be equipped
again for free, including while paused. Skins only change appearance: movement,
collision size and upgrades stay the same. Standard unlocks cost 30–735 coins;
ultra-premium unlocks cost 1,200–3,500 coins.

Unlocks and the equipped skin save alongside coins and upgrades. Older saves
load their existing currency and upgrades, and gain the default starter skins.
Skin names, prices and sprite rectangles are defined in `SkinManager.h` / `.cpp`.

## Built with

C++20, SDL 3.2.28, SDL_image 3.2.4, SDL_ttf 3.2.2, Box2D 3.1.1, CMake, and VS Code.

## Build and run

CMake downloads SDL3, its image/font libraries and Box2D automatically. The first build requires internet access.
Run these commands from the project folder.

### macOS

Requires Xcode Command Line Tools, Git, CMake 3.24+, and Ninja.

```sh
cmake --preset macos-debug
cmake --build --preset macos-debug
./build/macos-debug/game
```

### Windows

Requires Git, CMake 3.24+, and Visual Studio 2022 Build Tools with the
**Desktop development with C++** workload and a Windows SDK.

```powershell
cmake --preset windows
cmake --build --preset windows-debug
.\build\windows\Debug\game.exe
```

### VS Code

Install the recommended extensions, select your platform's CMake preset, and
configure the project. Choose the matching **Game** debugger configuration and
press **F5**.

## Code structure

- `main.cpp` — application entry point and error reporting.
- `Game.h` / `.cpp` — SDL resources, level setup, input dispatch, and the game loop.
- `Menu.h` / `.cpp` — menus, reusable UI slices, font rendering and button input.
- `RewardEffects.h` / `.cpp` — floating reward labels, sparks and effect lifetimes.
- `RunStats.h` — distance, current height and maximum height for each run.
- `RunResult.h` — payout rules and the frozen results breakdown.
- `UpgradeManager.h` / `.cpp` — upgrade levels, costs, purchases and gameplay effects.
- `ProgressStore.h` / `.cpp` — loading and saving coins, upgrades, skins and environment progression.
- `SkinManager.h` / `.cpp` — skin catalogue, unlocks, prices and equipped appearance.
- `EnvironmentManager.h` — environment catalogue, unlock targets, records and level settings.
- `LevelConfig.h` — assets, gravity, and launch settings for the current level.
- `Ball.h` / `.cpp` — ball physics, launch state, spin, and sprite rendering.
- `Launcher.h` / `.cpp` — launcher rendering, pull limits, and mouse controls.
- `CoinManager.h` / `.cpp` — coin chunks, pickups, drawing, and run resets.
- `TrampolineManager.h` / `.cpp` — floating trampolines, landing checks, and bounce rewards.
- `Camera.h` / `.cpp` — following the ball and converting screen/world positions.
- `GroundManager.h` / `.cpp` — creating and removing physics floor chunks.
- `BallSprites.h` — sprite rectangles for the ball designs.
- `Background.h` / `.cpp` — mountain/cloud parallax and altitude-based sky colours.

## References

- [SDL3 CMake integration](https://wiki.libsdl.org/SDL3/README-cmake)
- [SDL3 entry points](https://wiki.libsdl.org/SDL3/README-main-functions)
- [Box2D v3.1.1 source and documentation](https://github.com/erincatto/box2d/tree/v3.1.1)
