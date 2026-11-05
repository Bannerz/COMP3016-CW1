# AI development log

**Project:** Protolaunch — COMP3016 coursework  
**Log created:** 2 November 2026  
**Tool:** ChatGPT  
**Approach:** iterative development using AI for explanation, implementation and debugging

## Development approach

I defined Protolaunch as a slingshot progression game built around launching,
collecting coins, purchasing upgrades and improving performance. I used AI
substantially throughout development for technical explanations, architecture
suggestions, generated and edited C++/CMake code, debugging, documentation and
verification.

My contribution included establishing the game concept and feature requirements,
working through implementations with guidance, identifying problems during play,
and directing revisions to gameplay, presentation and progression. Some sessions
focused on explanations so I could implement a feature myself; others involved
requesting direct implementation or integration. The record below identifies both
forms of assistance.

Development followed an iterative process: introduce a feature, observe its
behaviour, identify what needed improving and refine it. Each entry records the
objective, a summary of the request, the AI contribution, subsequent refinement
and verification, and supporting references. Build and automated-check results
are distinguished from developer playtesting and relayed player feedback.

## Development record

### AI-01 — Project setup and portability

**Objective:** establish a C++/SDL3/Box2D project for development on macOS and
compilation on Windows.

**Prompt summary:** configure the workspace for VS Code, portable dependencies
and a usable build/debug workflow.

**AI contribution:** helped establish the CMake scaffold, dependency configuration,
platform presets and debugger setup, and explained step-by-step debugging.

**Refinement and verification:** I confirmed that debugging worked. I subsequently
reported a successful Windows build and run using Visual Studio 2022; this was
separate from the AI-run macOS checks.

**References:** `CMakeLists.txt`, `CMakePresets.json`, `.vscode/`; scaffold commit
`e07dd94`, 23 October 2026.

### AI-02 — Minimal starting point

**Objective:** start from a blank window and build the game incrementally.

**Prompt summary:** remove example gameplay, explain the main sections and mark
where new code should go; explain window resizing and background colour.

**AI contribution:** removed the sample game code, added explanatory comments and
provided guidance on SDL window flags and render-clear colour.

**Refinement and verification:** I confirmed the initial setup worked and chose a
fixed window with a sky-blue background as the starting presentation.

**References:** early window setup in `src/main.cpp`; later application and
rendering responsibilities moved into `src/Game.cpp`.


### AI-03 — Floor, ball and launch mechanics

**Objective:** introduce the repeating floor, sprite-sheet ball and pull/release
physics.

**Prompt summary:** explain how to implement these features, then help diagnose
build errors and repair the initial implementation with comments in my style.

**AI contribution:** provided implementation guidance and direct fixes, including
SDL_image target/include configuration and asset-copy paths.

**Refinement and verification:** the reported linker, missing-header and asset-copy
errors guided the fixes. Subsequent development used the working launcher and
ball as the basis for camera and collection mechanics.

**References:** `CMakeLists.txt`, `src/Ball.*`, `src/Launcher.*`; initial assets and
gameplay milestone `cafa814`, 24 October 2026.

### AI-04 — Camera following and coordinates

**Objective:** follow the launched ball on both axes with consistent world and
screen coordinates.

**Prompt summary:** explain horizontal and vertical following, then inspect the
changes after camera movement appeared inverted.

**AI contribution:** helped connect drag/release state and physics motion to the
camera, and corrected coordinate conversion and follow direction.

**Refinement and verification:** I reported the inverted movement and confirmed
the correction before continuing with longer runs. Camera behaviour was later
included in developer testing recorded in the report.

**References:** `src/Camera.*`, `src/Ball.*`, `src/Launcher.*`, `report.md`.

### AI-05 — Streamed coin generation

**Objective:** generate coins at the start of each run and support travel
horizontally and upward without a fixed populated world.

**Prompt summary:** explain random coin generation and extend it in both axes,
then integrate the approach directly.

**AI contribution:** first explained the design, then implemented streamed coin
regions and per-run collection history after I requested integration.

**Refinement and verification:** the scope expanded from a starting coin field to
regions generated around the active area. Later collection and long-run checks
are recorded in the report; distance-based thinning was added in AI-28.

**References:** `src/CoinManager.*`, `report.md`.

### AI-06 — Modular game structure

**Objective:** separate responsibilities into classes and leave `main.cpp` as the
entry point for future level expansion.

**Prompt summary:** extract the launcher and ball into their own classes and
organise the growing game into reusable systems.

**AI contribution:** helped refactor application coordination, ball, launcher,
camera, coins and streamed ground into separate source/header files.

**Refinement and verification:** the structure supported later environments,
progression and presentation features. The final ownership and dependency
relationships were checked when preparing the UML diagrams in AI-29.

**References:** `src/Game.*`, `src/main.cpp` and gameplay classes; `1611f3f`,
25 October, and `347c1aa`, 27 October 2026.

### AI-07 — Parallax scenery and sky

**Objective:** add depth to the background and communicate altitude through sky
colour.

**Prompt summary:** use the supplied cloud and mountain assets with subtle
parallax, then correct regular cloud placement and mountains lifting off the floor.

**AI contribution:** implemented and refined cloud distribution, size/depth
variation, mountain positioning and altitude-based sky blending.

**Refinement and verification:** my visual feedback identified both placement
problems. Cloud variation and ground-aligned mountain positioning were revised;
later developer scenery checks are recorded in the report.

**References:** `src/Background.*`, `src/LevelConfig.h`, `report.md`.

### AI-08 — Ground contact, resistance and spin

**Objective:** make ground travel predictable and bring the ball to rest naturally.

**Prompt summary:** add ground resistance and investigate unexpected bouncing at
chunk joins and spin stopping after ground contact.

**AI contribution:** helped refine rolling/contact behaviour and use Box2D chain
segments with ghost vertices so neighbouring floor sections form a continuous
surface without exposed collision corners at joins.

**Refinement and verification:** developer test DEV-04 observed no unexpected
seam bounce across multiple chunks. The same test found that spin could still
stop after ground contact and resume after a trampoline; this remains recorded
as BALL-01 rather than treated as resolved.

**References:** `src/GroundManager.*`, `src/Ball.*`; README P02–P04 and report
DEV-04/BALL-01.

### AI-09 — Coin tiers and trampolines

**Objective:** vary collection rewards and introduce rewarding bounce targets.

**Prompt summary:** use the three coin sprites with corresponding values, add
three trampoline sizes with equal thrust, and make smaller targets rarer and
more rewarding; pay trampoline rewards as coins.

**AI contribution:** implemented coin tiers, platform generation, landing checks,
one-time rewards and a damped compression animation. The three trampoline
variants use different widths, rewards and spawn weights with shared thrust.

**Refinement and verification:** I directed the move from a separate trampoline
score to the run coin total. Developer testing subsequently confirmed the three
variants, rewards and bounce animation.

**References:** `src/CoinManager.*`, `src/TrampolineManager.*`; report DEV-04.

### AI-10 — Menus, borders and anti-aliasing

**Objective:** create readable menus using the supplied UI slices and improve
rendering quality.

**Prompt summary:** add main/options menus and anti-aliasing, then try SDL's
built-in nine-slice renderer after border alignment differed from my mobile app.

**AI contribution:** implemented menu/rendering changes and refined the sliced
borders using `SDL_RenderTexture9Grid`.

**Refinement and verification:** my screenshot provided the alignment defect to
investigate. Menu navigation, options and visual presentation were subsequently
covered by the developer testing documented in the report.

**References:** `src/Game.*`, `src/Menu.*`, `report.md`.

### AI-11 — Run results and upgrades

**Objective:** connect performance to a final payout and repeatable progression.

**Prompt summary:** track distance, height and run time, produce an end-of-run
coin payout, and add a shop with upgrades; revise the starting launch power.

**AI contribution:** implemented statistics, payout calculations, results
navigation, banking and upgrade effects, then adjusted the starting tuning.

**Refinement and verification:** I requested scoring and balance changes to make
progression more interesting. Developer testing covered results and purchases;
a purchased boost applying on the next attempt was confirmed during testing.

**References:** `src/RunStats.h`, `src/RunResult.h`, `src/UpgradeManager.*`,
`src/Game.*`, `report.md`; progression milestone `dacfce1`, 31 October 2026.

### AI-12 — Cosmetic skin shop

**Objective:** provide cosmetic progression using the standard and ultra-premium
sprite sheets.

**Prompt summary:** add unlockable skins, a skin shop and the additional supplied
sprite sheet.

**AI contribution:** implemented the catalogue, previews, pagination,
purchase/equip flow and persistence, keeping cosmetic selection separate from
physics effects.

**Refinement and verification:** developer testing covered browsing, purchasing,
equipping and retaining selections, with outcomes recorded in the report.

**References:** `src/SkinManager.*`, `src/Menu.*`, `src/Ball.*`,
`src/ProgressStore.*`, `report.md`.

### AI-13 — Initial explicit save/load flow

**Objective:** let players deliberately save and restore progression alongside
autosave.

**Prompt summary:** explain the save location and format, then add explicit
save/load controls.

**AI contribution:** explained the plain-text `.dat` storage and implemented the
initial explicit flow through the game and menus.

**Refinement and verification:** this established the progression persistence
later expanded with independent named files, native dialogs and recovery in
AI-21. Persistence execution results are recorded in the report.

**References:** `src/ProgressStore.*`, `src/Game.*`, `src/Menu.*`, `report.md`.

### AI-14 — Unlockable environments

**Objective:** provide environments with distinct physics, performance targets
and free revisiting after unlocking.

**Prompt summary:** add levels unlocked by distance/height achievements, with
different physics properties and retained access to earlier areas.

**AI contribution:** implemented the six-area catalogue, records, unlock logic,
next-run selection and save migration, and proposed initial thresholds and
physics values.

**Refinement and verification:** developer testing covered environment access,
selection and persistence. The initial balance remained open to feedback, leading
to the editable settings file in AI-22.

**References:** `src/EnvironmentManager.h`, `src/ProgressStore.*`, `src/Game.*`,
`src/Menu.*`, `report.md`.

### AI-15 — Collection feedback

**Objective:** make coin-catcher range and earned rewards visible during play.

**Prompt summary:** outline the coin catcher and add `+N` effects for coin and
trampoline rewards.

**AI contribution:** implemented an upgrade-sensitive range outline, floating
reward labels and sparks, with pause/reset handling and a bounded active-effect
count.

**Refinement and verification:** developer testing covered collection and reward
feedback. The effects communicate changes to the existing run coin total.

**References:** `src/CoinManager.*`, `src/RewardEffects.*`, `src/Game.*`, `report.md`.


### AI-16 — Menu and launcher polish

**Objective:** improve presentation and make interface movement and launcher
tension more expressive.

**Prompt summary:** add a visual-polish pass to menus and launcher feedback.

**AI contribution:** added bouncing panel entrances, staggered controls and a
short anticipation/shrink/fade exit. Hit areas follow moving buttons, closing
suppresses repeated input, and longer labels fit the layout. Procedural bands
respond to pull tension and use upgrade-dependent colours/thicknesses.

**Refinement and verification:** navigation waits for closing animations while
purchases/options remain immediate. README L08 and M01–M04 cover the relevant
band, navigation and animated-input behaviour; later collar alignment was refined
separately in AI-24.

**References:** `src/Menu.*`, `src/Launcher.*`, `src/Game.*`; `63cf767`,
2 November 2026, with further menu refinements in `bd427c4`.

### AI-17 — Sound effects and music

**Objective:** provide responsive audio with independent music/effects controls.

**Prompt summary:** add gameplay and UI sounds, music transitions and adjustable
volume as part of presentation polish.

**AI contribution:** assisted with eight procedural 48 kHz mono PCM WAV effects
and an SDL mixer. Pull/rolling loops respond to tension/speed, impacts respond to
landing speed, and hover cues are rate-limited. Integrated supplied music with
looping, 0.8-second crossfades, playhead preservation and startup/exit fades.

**Refinement and verification:** loops fade/reset with gameplay state, controls
support muting, and unavailable audio devices leave gameplay usable. README
M05–M08 define audio coverage. The Mountain and HitsLab tracks were supplied;
the implementation did not generate that music.

**References:** `src/SoundEffects.*`, `src/Game.*`, `src/Menu.*`,
`tools/generate-sounds.mjs`; `bd427c4`, 2 November 2026; README music credits.

### AI-18 — Named saves and fresh-game recovery

**Objective:** support independent save files and recoverable progression resets.

**Prompt summary:** extend saving with native file dialogs, validated loading and
backup protection before starting fresh.

**AI contribution:** added Save as/Load file dialogs, extension/overwrite handling
and Unicode paths. Named files remain independent from autosave. Valid loading
discards a paused attempt before applying progression; invalid input preserves
the session. Fresh game creates a unique backup before resetting, cancelling if
backup or fresh-autosave creation fails.

**Refinement and verification:** successful loading with a later autosave failure
has distinct feedback. AI assistance also added persistence tests for round trips,
independent files, Unicode paths, backups and invalid/missing input. Subsequent
macOS execution of the existing `progress_store` CTest passed (1/1).

**References:** `src/ProgressStore.*`, `src/Game.*`, `src/Menu.*`,
`tests/ProgressStoreTests.cpp`; `bd427c4`, 2 November 2026; README S01–S12.

### AI-19 — Editable environment settings

**Objective:** make progression and physics values easy to adjust manually.

**Prompt summary:** move environment tuning into a dedicated C++ file following
progression feedback and document the change.

**AI contribution:** extracted the catalogue into `EnvironmentSettings.cpp` with
named, commented fields for unlock targets, physics, sky colour, asset suffix
and descriptions, and updated CMake.

**Refinement and verification:** existing values and environment IDs were
preserved. AI-run macOS builds of `game` and `progress_store_tests` succeeded,
and the existing CTest check passed (1/1). This was an organisational change;
subsequent balance decisions can be made directly in the settings file.

**References:** `src/EnvironmentSettings.cpp`, `src/EnvironmentManager.h`,
`CMakeLists.txt`; 4 November 2026.

### AI-20 — Altitude starfields

**Objective:** enrich high-altitude scenery in every environment.

**Prompt summary:** add stars that appear as the sky darkens with altitude.

**AI contribution:** implemented deterministic world-region starfields with
altitude fading, subtle parallax, varied brightness/size and occasional halos.
Per-environment colour and seed fields control appearance; only nearby regions
are drawn, without new sprite assets.

**Refinement and verification:** the macOS build passed. AI-run software-renderer
checks across all six environments covered daylight hiding, fading, repeatable
positions, seed variation, camera returns, descent and blend-state restoration;
captures were inspected. Developer test DEV-09 also confirmed the tested sky/star
presentation, without specifying the environment.

**References:** `src/Background.*`, `src/EnvironmentSettings.cpp`,
`src/EnvironmentManager.h`; report DEV-09; 4 November 2026.

### AI-21 — Launcher-band attachment correction

**Objective:** align bands with the collars on every launcher upgrade.

**Prompt summary:** investigate the level-4 attachment defect shown in a screenshot
and check the other launcher sprites.

**AI contribution:** replaced shared fork coordinates with per-sprite collar
points, keeping launcher artwork and band style independent. Checked six sprites
and all 36 launcher/band combinations, with rest/pull captures.

**Refinement and verification:** the macOS build and AI-rendered combination
checks passed. I subsequently reported that the alignment was much better;
this confirms the observed improvement without claiming a manual check of every
combination.

**References:** `src/Launcher.*`, `src/Game.cpp`; report LAUNCHER-01;
`docs/testing/launcher-bands-before.png` and `launcher-bands-after.png`;
4 November 2026.

### AI-22 — Persistent application preferences

**Objective:** retain volume and display options between sessions.

**Prompt summary:** save sound volume and other preferences across restarts.

**AI contribution:** added independent, versioned `preferences.dat` storage for
fullscreen, VSync, anti-aliasing and both volumes. Successful option changes save
immediately; startup applies saved mute before audio mixing. Validation prevents
partial application, and writes replace the file through a temporary file.

**Refinement and verification:** macOS builds and temporary AI checks passed for
combinations, endpoints, Unicode paths, replacement, malformed input, write
failures and restoration in a separate process. Loading/resetting progression
preserves preferences. Hardware/display and audible startup retests remain pending.

**References:** `src/Preferences.h`, `src/PreferencesStore.*`, `src/Game.*`,
`src/SoundEffects.*`, `src/Menu.h`, `CMakeLists.txt`; 4 November 2026.

### AI-23 — Speed feedback flame

**Objective:** help players judge speed during flight.

**Prompt summary:** record feedback that speed is difficult to determine and add
an animated flame whose tail follows the trajectory correctly.

**AI contribution:** integrated the supplied 20-frame sheet in `SpeedFlame`, using
velocity for activation/size and screen-space travel direction for rotation,
independent of ball spin. Added configurable thresholds, hysteresis and pause/reset
handling.

**Refinement and verification:** the macOS build passed. AI checks covered
activation, animation, pause/reset, unchanged velocity and trailing orientation
in eight rendered directions. I confirmed the initial cue helped and requested
the comet refinement below; this is separate from an external-player retest.

**References:** `src/SpeedFlame.*`, `src/LevelConfig.h`, `src/Game.*`, `report.md`,
`docs/testing/speed-flame-directions.png`; 4 November 2026.

### AI-24 — Comet envelope and speed-based growth

**Objective:** wrap the ball's leading edge and communicate increasing speed
through a smoothly growing effect.

**Prompt summary:** use the screenshot feedback to move the flame around the front
of the ball and make opacity/size increase with speed up to a limit.

**AI contribution:** moved the sprite head forward and introduced smoothed
speed-proportional intensity with editable length, opacity and offset settings.
The effect reaches its maximum at 100 m/s; the supplied sheet is unchanged.

**Refinement and verification:** the macOS build passed. Rendered checks confirmed
increasing brightness/width, identical capped images at 100 and 200 m/s, and
correct leading/trailing placement in eight directions. Animation, pause, reset
and fading checks passed; developer retesting of the refined appearance is pending.

**References:** `src/SpeedFlame.*`, `report.md`,
`docs/testing/comet-directions.png`, `docs/testing/comet-growth.png`;
4 November 2026.

### AI-25 — Distance-based resource scarcity

**Objective:** reduce support for excessively long runs while preserving stable
world generation.

**Prompt summary:** implement feedback recommending fewer coins and/or trampolines
as a run progresses.

**AI contribution:** added a configurable smooth retention curve per environment:
full density through 250 m, reducing to 35% coins and 10% platforms at 2,000 m.
Position-based thinning uses separate randomness to preserve retained layouts,
values and tiers; empty platform regions are cached and fixed coin seeds support
repeatable checks.

**Refinement and verification:** macOS builds passed. Fixed-seed samples retained
35.03% of coins and 10.10% of platforms. AI checks covered boundaries, early layouts,
stable subsets, revisits, collection/reward history, empty regions, zero minima
and all environments. Rewards/thrust are unchanged. Run-length/fairness retesting
is pending; repeat bounces on an existing platform remain possible.

**References:** `src/SpawnDensity.h`, `src/CoinManager.*`,
`src/TrampolineManager.*`, `src/EnvironmentSettings.cpp`, `report.md`;
4 November 2026.


### AI-26 — Shorter runs and lower payouts

**Objective:** make runs harder and shorter while slowing coin progression.

**Prompt summary:** inspect the current parameters, recommend a balance pass for
shorter runs and reduced payouts, then implement the suggested changes.

**AI contribution:** reduced trampoline thrust to 35 m/s and rewards to 3/7/18;
added per-platform repeat thrust of 100%/75%/50% with history preserved across
streaming. Brought scarcity forward to 150–1,200 m with 30% coin / 5% platform
minima, increased each environment's rolling resistance, and lowered payout
multipliers to a rounded combined cap of 9.38. Kept starting launch power,
upgrade prices and unlock targets unchanged. Updated documentation and added
focused balance regression tests.

**Refinement and verification:** macOS game and test builds passed, and CTest
passed 2/2. Automated checks covered all trampoline tiers, repeat thrust,
once-only rewards, unload/revisit persistence, new-run reset, crossing direction,
invalid settings, payouts and all six environments. Suggested duration/income
goals remain unmeasured; developer playtesting is needed to assess fairness,
progression and the existing rolling-spin defect.

**References:** `src/TrampolineManager.*`, `src/EnvironmentSettings.cpp`,
`src/SpawnDensity.h`, `src/RunResult.h`, `tests/BalanceTests.cpp`,
`CMakeLists.txt`, `README.md`, `report.md`; 4 November 2026.

## Asset acknowledgements

The game sprites, scenery, sprite sheets and UI slices were AI-generated by
Alex Banfield, who declares ownership of all rights to these generated visual
assets. The coding assistance recorded above integrated those supplied assets;
it did not create the supplied music or fonts. Sound effects were generated
procedurally through the AI-assisted script described in AI-20.

Google Fonts supplied Fredoka and Lilita One, whose SIL Open Font License 1.1
texts are retained in the repository. Music sources, visual-asset provenance and
font licence links are consolidated in the
[README acknowledgements](README.md#asset-provenance-and-licences).

I remain responsible for the submitted project's correctness, integration,
maintainability, originality and evaluation. This log records how AI assistance
contributed to the work alongside my design direction and iterative refinement.
