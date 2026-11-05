# Protolaunch

Protolaunch is a slingshot upgrade game being developed in C++ for COMP3016.
Drag an object back, release it to launch, collect coins, and spend them on
upgrades to fly further and earn bigger payouts.

The [development report](report.md) outlines proposal fulfilment, technical design,
workflow, AI integration, testing and evaluation.

[Watch the demonstration and code walkthrough](https://youtu.be/iLoCSkvSo98).

## Gameplay

**Launch → collect coins → earn a payout → upgrade → fly further.**

- Aim and launch the object using a mouse-driven slingshot.
- Procedural rubber bands follow the pull, thinning and brightening as tension increases.
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
also offers Resume and End run. Options controls fullscreen, VSync, anti-aliasing,
music volume and effects volume; changes
apply immediately and save automatically across sessions.
Preferences are stored as plain text in `preferences.dat`, alongside `progress.dat`
in the Protolaunch preferences folder. Fullscreen, VSync, anti-aliasing and both
volume levels are saved after each successful change. Missing files use defaults;
invalid files are ignored with feedback. Loading a game or using Fresh game keeps
these preferences.
Use the music and effects minus/plus buttons to adjust each volume in 10% steps;
0% mutes that channel. Effects volume includes gameplay and menu button sounds.

Menu panels pop in with a short bounce, followed by staggered button bounces.
The animation also plays when reopening a menu or changing skin pages, and mouse
hit areas follow the moving buttons.
Menu layouts reserve space for footer hints, panel borders and selection arrows;
long labels and currency values fit within their allotted width.
Closing a menu gives it a small anticipation bounce before it shrinks and fades
away over 0.22 seconds. Navigation, Resume, Play and Quit complete after the close;
repeated input during the transition is ignored. Purchases and option toggles remain immediate.

The HUD tracks your furthest horizontal distance from the launcher, current
height above the ground and highest altitude this run, all in metres. Measurements
start on launch, remain when paused and reset with a new run. Run time counts
active play after launch, including rolling, and excludes pauses and menu time.

Above 35 m/s, an animated comet flame gradually becomes brighter and larger with
speed. Its leading rim wraps around the front of the ball, while the tail follows
the trajectory rather than the skin's spin. Size and opacity reach their limits
at 100 m/s; further speed does not enlarge the effect. It fades as the ball slows,
pauses with the game and clears on a new run. Tune thresholds, smoothing, maximum
size/opacity and leading-edge placement in `SpeedFlameSettings` inside
`src/SpeedFlame.h`.

## Environments

Open **Environments** from the main, pause or results menu. Yard is available at
the start. Unlock each following area by reaching **either** target in the previous
area; milestones are recorded when the run ends, including runs ended with R.
Personal distance and height records belong to each environment, and never decrease.

| Environment | Target in previous area | Gravity (m/s²) | Ground grip | Rolling resistance |
| --- | --- | --- | --- | --- |
| Yard | Available from the start | 9.8 | 0.9 | 0.55 |
| Coast | Yard: 250 m distance OR 60 m height | 8.5 | 0.6 | 0.35 |
| Peaks | Coast: 600 m distance OR 180 m height | 12.0 | 0.3 | 0.14 |
| Storm | Peaks: 1,000 m distance OR 350 m height | 10.8 | 0.8 | 0.45 |
| Orbit | Storm: 1,600 m distance OR 600 m height | 3.2 | 0.45 | 0.20 |
| Void | Orbit: 2,500 m distance OR 1,000 m height | 5.5 | 0.2 | 0.12 |

Each area uses its matching ground, mountain and cloud artwork, plus its own sky
colour. Stars fade in as the sky darkens at altitude and fade out on descent,
with subtle parallax and a stable layout/tint for each area. Higher gravity shortens flights; lower grip and rolling resistance make
surfaces more slippery. Upgrades, skins and currency carry across every area.

Unlocked areas stay available and can be revisited freely. Selection applies to
the next new run; Resume continues the paused run in its original environment.
The HUD names the active environment. Automatic and manual saves include records
and the selected area. Older saves retain existing progression and start in Yard.
Tune environment names, unlock targets, gravity, ground resistance, asset sets and
sky colours in `src/EnvironmentSettings.cpp`, then rebuild. Each environment uses
named fields so the values can be edited individually. `starColour` and `starSeed`
control each area's star tint and layout.

### Distance-based scarcity

Coin and trampoline layouts become sparser farther from the launcher. The first
150 metres keep the original density. A smooth curve reduces retained placements
to 30% for coins and 5% for trampolines at 1,200 metres, with those minimum rates
continuing beyond that distance. Rates are probabilities, not exact per-region counts.

These values are editable per environment through `coinDensity` and
`trampolineDensity` in `src/EnvironmentSettings.cpp`: `startDistance`, `endDistance`
and `minimumDensity`. A minimum of 1 disables thinning; 0 removes new placements
beyond the end distance. Locations, coin values and trampoline tiers remain stable
when revisiting streamed regions. Thinning does not remove objects during approach,
change bounce thrust or alter reward values.

This is a balance measure intended to reduce long chains of replenishing platforms;
it does not impose a hard time limit. Repeat bounces on the same platform have
reduced thrust: 100% on the first landing, 75% on the second and 50% thereafter.
Fresh platforms provide 35 m/s upward speed for all three tiers. Landing history
persists when regions unload and resets for a new run. These values are editable
in `TrampolineSettings` in `src/TrampolineManager.h`.

## Run payouts

Runs finish when the ball rests on the ground for 0.75 seconds, or when you
choose End run or press R during flight. The results screen shows your run coins,
distance, maximum height, run time and final payout, with Play again, Shop and Main menu
buttons. Trampolines award 3, 7 or 18 coins directly to the run counter, once per
platform per run.

**Payout = run coins × distance multiplier × height multiplier × time multiplier.**

Each multiplier starts at ×1: distance adds ×1 per 400 metres (up to ×3), maximum
height adds ×1 per 150 metres (up to ×2.5), and time adds ×1 per 120 seconds (up to ×1.25).
The combined multiplier caps at ×9.38 after rounding.
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

Bands have a distinct colour and increasing thickness at each level: tan at level 0,
green at 1, cyan at 2, blue at 3, violet at 4 and gold at 5. Their visual style applies
on the next run along with the increased pull distance.

Air boosts add upward speed while keeping horizontal motion. Charges refill
on a new run. The HUD shows the charges remaining.

The Coin Magnet shows a gold outline around its pickup area, growing with each
upgrade level. Coin pickups produce gold `+1`, `+2` or `+3` labels and sparks;
paid trampoline bounces produce mint `+3`, `+7` or `+18` effects. Feedback rises
and fades, pauses with the game and clears when starting a new run.

Coins and purchased levels save automatically to `progress.dat` in SDL's
Protolaunch preferences folder, outside the build and asset folders. Copy this
file to the corresponding preferences folder to move progression between machines.
Choose **Save / Load** from the main or pause menu. **Save as...** opens the native
file dialog to name and store independent `.dat` saves; **Load file...** opens a
file picker. Save as many files as you like and choose any of them later. Named
saves are separate from automatic `progress.dat` saves and are only changed when
you explicitly save over them. Cancelled dialogs and invalid files leave current
progress intact. Loading ends any paused run, restores coins, upgrades, skins and
environments, and updates autosave. Runs are not saved. Application options are
stored separately and are not replaced by loading progression.

**Fresh game** opens a confirmation screen. Confirming first creates a uniquely
named `progress-backup-*.dat` in the preferences folder, then resets coins,
upgrades, skins and environment records to the starter progression in Yard.
Existing named saves and backups remain available through **Load file...**.
If backup or autosave fails, the reset is cancelled and current progression stays intact.
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

## Sound effects

Original procedural effects in `src/assets/audio` cover rubber pull, release twang,
coin chimes, trampoline springs, ground impacts and rolling. Pull pitch follows tension;
movement makes the creak louder. Impact volume follows landing speed, and rolling
volume and pitch follow horizontal speed. Loops fade out on pause or release, and
reset with each run. Missing audio devices leave the game playable without sound.

Menu buttons have a quiet hover pip and a soft activation pop, for mouse and keyboard
input. Disabled buttons stay silent; hover cues are rate-limited to avoid chatter.
Rolling sits quietly beneath the other effects.

Regenerate the eight 48 kHz mono PCM WAV files with `node tools/generate-sounds.mjs`.
No external samples, audio dependencies or online services are needed.

Music uses the supplied WAV tracks: **8 Bit Retro by The Mountain** in menus (including pause,
shops and results) and while aiming, then **Retro Arcade Game Music by Hitslab** starts when the ball launches.
Tracks loop in stereo and crossfade over 0.8 seconds on launch and when returning
to menus. Music fades
in on startup and out before exit; menu navigation and run resets keep playback
continuous. Fully faded tracks retain their position for the next transition.

## Music acknowledgements

- Menu and aiming music by [The_Mountain](https://pixabay.com/users/the_mountain-3616498/), sourced from Pixabay.
- Gameplay music by [HitsLab](https://pixabay.com/users/hitslab-47305729/), sourced from Pixabay.

## Asset provenance and licences

| Asset | Source / creator | Rights and licence record |
| --- | --- | --- |
| Game sprites, sprite sheets and scenery | AI-generated by Alex Banfield for Protolaunch. | Alex Banfield declares ownership of all rights to these generated assets. |
| UI artwork and nine-slice panels | AI-generated by Alex Banfield for Protolaunch. | Alex Banfield declares ownership of all rights to these generated assets. |
| Fredoka font | [Google Fonts — Fredoka](https://fonts.google.com/specimen/Fredoka); copyright The Fredoka Project Authors. | SIL Open Font License 1.1; [bundled licence](src/assets/fonts/Fredoka/OFL.txt). |
| Lilita One font | [Google Fonts — Lilita One](https://fonts.google.com/specimen/Lilita+One); copyright Juan Montoreano. | SIL Open Font License 1.1; [bundled licence](src/assets/fonts/Lilita_One/OFL.txt). |
| Music | The Mountain and HitsLab, sourced from Pixabay. | Sources are recorded in the [music acknowledgements](#music-acknowledgements). |
| Sound effects | Procedurally synthesised for the project using the AI-assisted `tools/generate-sounds.mjs` script. | Original generated effects; no external audio samples used. |

The font copyright notices and full licence texts are included with the font
assets and copied into the runtime assets folder by the build.

## AI assistance

Protolaunch was developed through an iterative process combining my game design,
feature requirements and play-driven refinements with AI assistance
in explanations, debugging, architecture, implementation, documentation and checks.
The [AI development log](AI_DEVELOPMENT_LOG.md) records these contributions,
key development decisions and the verification approach. The
[signed Generative AI Declaration](Generative%20AI%20Tool%20declaration.pdf)
records Partnered Work (P1) and is included as a submission appendix.

## Testing and playtesting

Testing covers gameplay, physics, progression, save recovery, portability,
presentation and usability. The cases below define the planned checks and expected
behaviour; they are not a list of completed passes.

### Retrospective: ground chunk seam bounce

**Observed problem:** during a long ground roll, the ball sometimes bounced upward
without hitting a trampoline. The issue was traced to joins between streamed
floor sections, where separate collision edges exposed corners.

**Change made:** the floor now uses Box2D chain segments with ghost vertices past
each end. Neighbouring chains share three points, describing a continuous flat
surface so contact at a join does not act like hitting a corner. Floor sections
are streamed around the ball as it travels.

**Evidence status:** development fix, supported by the current
`GroundManager.cpp` implementation. No original date, video or formal result was
recorded.

### Build, startup and portability

| ID | Steps / setup | Expected behaviour |
| --- | --- | --- |
| B01 | Configure, build and launch the current commit on macOS using the documented preset. | Build succeeds; main menu, sprites, fonts and audio load; no startup crash. |
| B02 | Build a clean checkout on Windows using the documented preset, then launch it. | Build and launch succeed without machine-specific macOS paths; native dialogs and assets work. |
| B03 | Put a checkout/build in a path containing spaces; also try a Unicode folder name. Launch from a different working directory. | Runtime assets resolve relative to the executable, independently of the working directory. |
| B04 | In a disposable build, remove a required image or font and launch. Restore it afterwards. | Startup reports a useful error and exits cleanly; no unexplained crash or progress damage. |
| B05 | Start with no existing preferences/save file; then restart with an existing save. | First launch uses starter progression; the next restores saved progression without requiring assets in the save directory. |

### Launcher and unexpected input

| ID | Steps / setup | Expected behaviour |
| --- | --- | --- |
| L01 | Drag the ball in several directions and release; include a short pull and full pull. | Ball follows the bounded drag and launches opposite the pull; stronger pulls produce more launch speed. |
| L02 | Click outside the ball; drag on scenery, menus and empty space. | These clicks do not grab or launch the ball. |
| L03 | Pull far beyond the maximum radius; release near the ball's starting position. | Pull stays capped; near-zero pulls remain stable; no extreme speed or invalid physics. |
| L04 | Drag, move outside the window and release; repeat after changing focus to another application. | Input is released or safely cancelled; no permanently stuck drag or captured mouse. |
| L05 | Press Escape while dragging; resume. Repeat while flying and rolling. | Drag cancels safely; paused physics and timer freeze; resuming does not cause a time jump. |
| L06 | Press R before launch, then during flight and rolling. | Before launch, resets the attempt; after launch, finishes it and awards the payout once. |
| L07 | Rapidly click/release, hold R, hold Escape, press unrelated keys and press Up with no boost unlocked. | No crash, repeated payout, unintended menu activation or free air boost. |
| L08 | Inspect rubber bands at every Bands level, both resting and fully pulled; cancel and reset. | Bands stay attached, follow the pull, change tension appearance and use the correct level colour/thickness; no stranded band geometry. |

### Physics, floor streaming and camera

| ID | Steps / setup | Expected behaviour |
| --- | --- | --- |
| P01 | Launch onto flat ground and watch initial impact, rolling and rest. | Initial impact may bounce; spin continues after contact, resistance slows the ball and sustained rest ends the run. |
| P02 | After the initial landing, observe a long roll over at least five floor joins on a path without trampoline contacts. Repeat with consistent environment and upgrade settings. | No new upward kick, snag or fall through the floor at a join. Ordinary first-impact bounce is not counted as a seam failure. |
| P03 | Repeat P02 at lower and higher horizontal speeds and on grippy and slippery environments. | Continuous ground contact remains stable across different contact speeds and resistance settings. |
| P04 | Travel far enough for old ground chunks to unload and new ones to appear; also try a leftward launch. Use a controlled physics check for reverse travel if normal input cannot reproduce it. | Ground remains continuous on both sides of the origin and during chunk creation/removal; no gaps or seam kicks. |
| P05 | Fly right, rise high and descend to the ground. | Camera follows both axes in the correct direction; sprites, collision positions and scenery remain aligned. |
| P06 | Reach high altitude, descend, pause and resume. | Sky darkens smoothly with altitude, clouds vary in size/depth and mountain bases remain attached to the ground. |
| P07 | Compare equal controlled launches in all six environments with the same upgrades. | Flight acceleration and ground behaviour match the configured environment; upgrades combine with its gravity correctly. Use controlled input for numeric comparisons. |
| P08 | Use an air boost during ascent and descent until charges run out; pause and start a new run. | Each accepted press uses one charge and adds upward speed while preserving horizontal motion/spin; pause gives no refill and a new run refills charges. |

### Coins, catcher and feedback

| ID | Steps / setup | Expected behaviour |
| --- | --- | --- |
| C01 | Collect each coin type separately. | Run balance increases by exactly 1, 2 or 3, matching the sprite and gold floating label. |
| C02 | Remain near a collected coin, leave its chunk and return during the same run. | The coin stays collected; no duplicate reward, chime or particle burst. |
| C03 | Start several new runs; travel horizontally and high vertically. | New layouts appear between runs; coins keep generating nearby in both dimensions without a visible end to the field. |
| C04 | Compare no Magnet with levels 1–5; approach a coin just outside and just inside the outlined catcher. | No upgraded ring at level 0; ring grows with levels and corresponds to collection when a coin's edge touches the catcher. |
| C05 | Collect several nearby coins rapidly, including mixed values. | Every value is credited once; labels remain readable, rise and fade; sparks clear without lasting clutter. |
| C06 | Pause while feedback is visible, resume and start a new run; repeat with anti-aliasing off. | Effects freeze while paused, continue on resume and clear on reset; both graphics settings render them correctly. |
| C07 | Compare Lucky Loot at level 0 and 5 over a large recorded sample or controlled generation check. | Higher levels increase relative chances of higher-value coins; do not infer a probability defect from a few unlucky runs. |
| C08 | Compare seeded coin generation before 150 m, through the transition and beyond 1,200 m; leave and revisit regions. | Early density is retained, later samples thin towards 30%, and retained coins keep their positions/values and collected state. |

### Trampolines

| ID | Steps / setup | Expected behaviour |
| --- | --- | --- |
| T01 | Land from above on each of the three trampoline types. | Green/blue/purple award 3/7/18 run coins and matching mint labels; all set the same upward launch speed. |
| T02 | Hit a platform from below, pass beside it and fall fast across it. | Below/side passes do not trigger a landing; a valid fast downward crossing is detected. |
| T03 | Bounce on the same platform twice, then revisit after its chunk reloads. | Thrust falls from 35 to 26.25 to 17.5 m/s on the first, second and subsequent landings; reloading preserves this history. Each platform pays only once per run. |
| T04 | Watch a bounce animation; hit again before it has settled. | Platform compresses and oscillates back to rest; animation does not shift its collision surface or leave it permanently displaced. |
| T05 | Observe many generated platforms, then reset the run. | Smaller valuable tiers are rarer over a sufficient sample; new runs reset reward and thrust history and produce a fresh layout. |
| T06 | Compare seeded platform generation before 150 m and beyond 1,200 m; revisit populated/empty regions and bounce on retained platforms. | Late samples thin towards 5%; regions stay stable, repeat-bounce thrust history persists and each platform still pays only once per run. |

### Run measurements and payouts

| ID | Steps / setup | Expected behaviour |
| --- | --- | --- |
| R01 | Complete a run with coins; navigate Results → Shop → Results, then start again. | One payout is credited; revisiting screens does not pay twice; the next run starts with zero run coins and measurements. |
| R02 | Pause for a measured interval during flight; compare HUD time before/after. | Run time excludes menus/pause and includes active rolling; aiming before launch does not increase it. |
| R03 | Rise and descend; travel forward then backward where possible. | Current height falls on descent, maximum height never decreases within the run, and furthest distance is not reduced by backtracking. |
| R04 | End a run with zero coins; separately let a ball rest naturally. | Zero coins produce zero payout; resting for 0.75 seconds ends the run, while a brief slow contact does not. |
| R05 | Controlled reward fixture: 10 coins, 400 m distance, 150 m height, 60 s. | Multipliers are ×2, ×2 and ×1.25; total ×5 and payout 50 coins. |
| R06 | Controlled fixture: 7 coins, 200 m distance, 75 m height, 30 s; also exceed every multiplier cap. | Fractional example shows ×1.5/×1.5/×1.25, combined ×2.81 and payout 19. Caps are ×3/×2.5/×1.25, combined ×9.38. Display and integer payout agree. |

### Upgrades and skins

| ID | Steps / setup | Expected behaviour |
| --- | --- | --- |
| U01 | Buy each upgrade with sufficient funds and start a new run. | Correct price is deducted once; level increases once; the advertised effect applies. |
| U02 | Attempt purchases with insufficient coins, exactly enough coins and at maximum level. | Unaffordable/maxed purchases change nothing; exact funds permit purchase and leave zero; five-level cap and doubled prices are respected. |
| U03 | Buy an upgrade while paused, then Resume and later start a new run. | Existing run keeps its original effects and boost allowance; the next run uses the new level. |
| U04 | Compare Launcher, Bands and Glider at levels 0 and 5 using repeatable launch input. | Power, maximum pull, launcher artwork, band appearance and gravity changes match their definitions. |
| U05 | Browse every standard and ultra-premium skin page with mouse and keyboard. | All 64 previews, names, prices, page limits and collection tabs work; no cropped/out-of-bounds sprites or hidden buttons. |
| U06 | Buy a locked skin, equip another owned skin and select the already-equipped skin. | First purchase charges once and unlocks/equips; owned skins are free; already-equipped selection has no charge. |
| U07 | Equip a different skin while paused mid-flight, then resume. | Appearance changes; position, velocity, spin, collision size and run measurements do not. |
| U08 | Save/reload after buying upgrades and skins in alternating order. | Neither type of purchase erases the other progression; starter skins remain available. |

### Environment progression

| ID | Steps / setup | Expected behaviour |
| --- | --- | --- |
| E01 | Start fresh and select each locked area using mouse and keyboard. | Only Yard is initially available; locked cards cannot start a run or alter selection. |
| E02 | For each next-area target, test just below both limits, exact distance with height below target, and exact height with distance below target. Use controlled fixtures for exact boundaries. | Below both remains locked; reaching either threshold in the preceding area unlocks it when the run ends. |
| E03 | Reach a large record in Yard and attempt to skip directly to later areas. | Only Coast unlocks from Yard; each following target belongs to its own preceding environment. |
| E04 | Unlock an area, finish a worse run in the preceding area, revisit Yard and reload the save. | Best records do not decrease and unlocked environments stay selectable. |
| E05 | Choose another area while paused, Resume, then finish and Play again. | Resume keeps the original area; next run loads the selected art/physics and the HUD names that area. |
| E06 | Open environment selection from Results and return several times. | Navigation returns to the correct screen without another payout, record reset or forced run start. |

### Save/load and recovery

| ID | Steps / setup | Expected behaviour |
| --- | --- | --- |
| S01 | Finish a run, buy an upgrade/skin, select an environment, quit and reopen. | Autosave restores the bank, all levels, ownership, equipped skin, records and environment choice. |
| S02 | Save two named files with different progression, then load each. | Files remain independent; each restores its own values. Loading also updates autosave; later automatic saves do not overwrite independent named files. |
| S03 | Cancel Save as and Load file dialogs, including while a run is paused. | No progression or run-state change; no stuck modal state or accidental gameplay input. |
| S04 | Save using spaces/Unicode in names; test names with and without `.dat`, and overwriting an existing save on both platforms. | Correct target file is written; extension handling and overwrite confirmation protect the intended file. |
| S05 | Load an empty, truncated, unrelated or unsupported-version file. | Clear failure feedback; current session and autosave remain intact; no partial application or crash. |
| S06 | Controlled invalid saves: negative/excessive currency, invalid upgrade levels, malformed skin ownership, invalid equipped/selected IDs and non-finite/inconsistent records. | Invalid progression is rejected as a whole; valid current progression remains unchanged. |
| S07 | In a disposable setup, deny writes or use a missing destination; try saving and purchases, and a named load whose autosave cannot be written. | Failure is reported; existing disk save remains readable and failed purchases roll back. If a valid named file loads but autosave fails, the loaded session remains active with an explicit autosave-failure message. |
| S08 | Load supported version 1 and version 2 fixtures, then save/reopen them. | Existing coins/upgrades survive; legacy defaults are supplied for newer fields; rewritten saves load successfully. |
| S09 | Load a valid file during a paused run. | Current run ends without an extra payout; loaded progression replaces it and the next run uses loaded settings. |
| S10 | Open Fresh game, cancel, then confirm in a separate attempt. | Cancel changes nothing; confirm creates a unique backup before resetting to starter Yard progression. Named saves remain intact. |
| S11 | Restore the fresh-game backup; repeat reset twice; simulate backup/autosave failure in a disposable setup. | Backups are distinct and restorable; failed reset preserves current progression. |
| S12 | Copy a named `.dat` save between macOS and Windows and load it. | Progression is portable; no saved machine-specific asset paths are required. |

### Menus, rendering and sound

| ID | Steps / setup | Expected behaviour |
| --- | --- | --- |
| M01 | Navigate every menu with mouse, arrows, Tab/Shift+Tab, Enter and Space. | Enabled controls work, disabled controls are skipped, selection stays valid and Back/Escape return appropriately. |
| M02 | Click moving buttons during menu opening; rapidly repeat Play, Back, Resume and Quit during closing. | Hit areas follow animation; one navigation action completes after the close; repeated input does not duplicate actions or leak into gameplay. |
| M03 | Browse skin pages and reopen menus repeatedly. Inspect panels at fullscreen/windowed sizes and with large bank values. | Animations replay correctly; nine-slice borders align and labels, focus arrows and footer hints fit. |
| M04 | Toggle fullscreen, VSync and anti-aliasing; pause/resume, then relaunch. | Session settings apply or report an unsupported option; coordinates remain correct. Preferences restore on relaunch; unsupported display settings fall back with feedback. |
| M05 | Change music/effects volume independently, including 0% and 100%. | Each channel responds independently in 10% steps; limits hold and effects mute includes menu sounds. |
| M06 | Pull/release, collect coins, hit trampolines, land and roll at different speeds. | Appropriate sounds play once per event; tension/speed change the intended sounds; loops end or fade correctly on pause/reset. |
| M07 | Launch, pause, visit menus, resume and finish; listen across repeated transitions. | Menu/aiming and flight tracks loop and crossfade without stacking, restarting on every button press or abrupt gaps. |
| M08 | Hover rapidly, navigate with keys and try disabled buttons; launch without an available audio device in a controlled setup. | Hover is rate-limited, disabled buttons stay silent and missing audio leaves the game playable. |

### Endurance and player feedback

| ID | Scenario | Expected behaviour / evaluation criteria |
| --- | --- | --- |
| Q01 | A 20–30 minute session with long flights and repeated resets; frame-rate and memory comparisons at equivalent points. | No crash, growing slowdown or persistent effect clutter; per-run resources recover after resets. |
| Q02 | Repeatedly switch environments, buy/equip skins, open/cancel file dialogs and toggle graphics settings. | No stale resources, broken controls, lost progression or accumulating audio loops. |
| Q03 | A new player's first 10–15 minutes: launching, earning coins and buying an upgrade with minimal guidance. | Usability measures include time to first launch/purchase, unclear labels, mistaken assumptions and requests for help. |
| Q04 | A new player unlocks/selects another area, returns to Yard, saves and reloads. | Unlock targets, permanent access, next-run selection and save controls are understandable without additional explanation. |
| Q05 | Player feedback on aiming, speed, reward feedback, difficulty, upgrade value, sound balance and replayability. | Evaluation identifies satisfying features, confusing interactions, frustrating waits and perceived unfairness. |
| Q06 | Repeat a player task after a change addressing a specific usability issue. | A before/after comparison establishes whether the issue improved and identifies any remaining problems. |

## Built with

C++20, SDL 3.2.28, SDL_image 3.2.4, SDL_ttf 3.2.2, Box2D 3.1.1, CMake, and VS Code.

## Windows release download

Download [Protolaunch Release V1](Protolaunch%20Release%20V1.zip) (76.9 MiB), then:

1. Extract the ZIP completely.
2. Open the extracted `Protolaunch Release V1` folder.
3. Run `game.exe`, keeping the `assets` folder beside it.

Visual Studio and CMake are not needed to play the packaged build. The executable
requires the **Microsoft Visual C++ v14 x64 runtime**. If Windows reports missing
`MSVCP140.dll` or `VCRUNTIME140.dll`, install the x64 Redistributable from
[Microsoft's official download page](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170).

I have confirmed that this exact archive was extracted on Windows and
`game.exe` launched and ran successfully outside Visual Studio.

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

### Windows — Visual Studio 2022

Install Git and Visual Studio 2022 with the **Desktop development with C++**
workload, including **C++ CMake tools for Windows** and a Windows SDK.

1. Clone or download the repository, then open its root folder in Visual Studio
   using **File → Open → Folder**. Open the folder containing `CMakeLists.txt`.
2. Allow CMake to configure the project and download/build its dependencies.
   No separate SDL3 or Box2D installation is needed. The first setup takes longer
   and requires internet access.
3. Select the Windows configuration/preset and choose **game.exe** as the startup
   target if it is not already selected.
4. Press the green **Run** button or **F5** to build and launch the game.

For a Release build, select **Release** / the `windows-release` build preset and
build the `game` target. With the supplied Windows preset, the executable is
`build/windows/Release/game.exe`; its `assets` folder is copied alongside it.
Keep that folder beside the executable when running it outside Visual Studio.

**Verified on Windows:** the developer opened the project in Visual Studio 2022,
allowed dependencies to configure, and successfully built and ran the game.
A Release build also compiled without errors.

Alternatively, from a Visual Studio developer terminal with CMake 3.24+:

```powershell
cmake --preset windows
cmake --build --preset windows-debug
.\build\windows\Debug\game.exe
```

To compile Release from the terminal:

```powershell
cmake --build --preset windows-release
.\build\windows\Release\game.exe
```

### VS Code

Install the recommended extensions, select your platform's CMake preset, and
configure the project. Choose the matching **Game** debugger configuration and
press **F5**.

## Code structure

[Implemented UML class diagrams](docs/uml.md) show ownership, gameplay dependencies,
progression and configuration.

- `main.cpp` — application entry point and error reporting.
- `Game.h` / `.cpp` — SDL resources, level setup, input dispatch, and the game loop.
- `Menu.h` / `.cpp` — menus, reusable UI slices, font rendering and button input.
- `RewardEffects.h` / `.cpp` — floating reward labels, sparks and effect lifetimes.
- `SpeedFlame.h` / `.cpp` — speed-triggered sprite animation aligned to the ball trajectory.
- `RunStats.h` — distance, current height and maximum height for each run.
- `RunResult.h` — payout rules and the frozen results breakdown.
- `UpgradeManager.h` / `.cpp` — upgrade levels, costs, purchases and gameplay effects.
- `ProgressStore.h` / `.cpp` — loading and saving coins, upgrades, skins and environment progression.
- `Preferences.h`, `PreferencesStore.h` / `.cpp` — application options and independent persistent settings.
- `SkinManager.h` / `.cpp` — skin catalogue, unlocks, prices and equipped appearance.
- `EnvironmentManager.h` — environment records, unlock rules and level settings.
- `EnvironmentSettings.cpp` — editable environment catalogue, unlock targets, physics, appearance and spawn-density curves.
- `SpawnDensity.h` — shared smooth distance-based spawn retention.
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

The report includes [technical documentation and programming-pattern references](report.md#technical-documentation) supporting the implementation discussion.

- [SDL3 CMake integration](https://wiki.libsdl.org/SDL3/README-cmake)
- [SDL3 entry points](https://wiki.libsdl.org/SDL3/README-main-functions)
- [Box2D v3.1.1 source and documentation](https://github.com/erincatto/box2d/tree/v3.1.1)
