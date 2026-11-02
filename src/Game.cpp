#include "Game.h"
#include "Ball.h"
#include "Launcher.h"
#include "CoinManager.h"
#include "GroundManager.h"
#include "Background.h"
#include "TrampolineManager.h"
#include "ProgressStore.h"
#include "RewardEffects.h"
#include "SoundEffects.h"

#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <mutex>

struct Game::ProgressDialog {
    std::mutex mutex;
    bool ready = false;
    bool failed = false;
    bool saving = false;
    std::string path;
    std::string defaultLocation;
};

namespace {
std::string pathText(const std::filesystem::path& path)
{
    const auto utf8 = path.u8string();
    return {utf8.begin(), utf8.end()};
}
}

Game::Game() = default;

Game::~Game()
{
    //destroy level objects before their world and textures
    rewardEffects.reset();
    sounds.reset();
    menu.reset();
    unloadLevel();
    renderer.reset();
    window.reset();
    if (fontsReady) {
        TTF_Quit();
    }
    if (sdlReady) {
        SDL_Quit();
    }
}

void Game::initialize()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("SDL initialization failed: ") + SDL_GetError());
    }
    sdlReady = true;
    if (!TTF_Init()) {
        throw std::runtime_error("Font initialization failed: " + std::string(SDL_GetError()));
    }
    fontsReady = true;

    SDL_Window* newWindow = nullptr;
    SDL_Renderer* newRenderer = nullptr;
    const bool created = SDL_CreateWindowAndRenderer(
        "Protolaunch", windowWidth, windowHeight, SDL_WINDOW_HIGH_PIXEL_DENSITY,
        &newWindow, &newRenderer
    );
    window.reset(newWindow);
    renderer.reset(newRenderer);
    if (!created) {
        throw std::runtime_error(std::string("Window/renderer creation failed: ") + SDL_GetError());
    }

    //use the same drawing coordinates on both platforms
    if (!SDL_SetRenderLogicalPresentation(renderer.get(), windowWidth, windowHeight,
                                           SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
        throw std::runtime_error(std::string("Drawing area setup failed: ") + SDL_GetError());
    }

    const char* basePath = SDL_GetBasePath();
    if (!basePath) {
        throw std::runtime_error(std::string("Failed to get base path: ") + SDL_GetError());
    }
    assetDirectory = std::string(basePath) + "assets/images/";
    menu = std::make_unique<Menu>(renderer.get(), std::string(basePath) + "assets/");
    rewardEffects = std::make_unique<RewardEffects>(renderer.get(), std::string(basePath) + "assets/");
    sounds = std::make_unique<SoundEffects>(std::string(basePath) + "assets/");
    options.vsync = SDL_SetRenderVSync(renderer.get(), 1);
}

Game::TexturePtr Game::loadTexture(const std::string& filename)
{
    const std::string path = assetDirectory + filename;
    TexturePtr texture(IMG_LoadTexture(renderer.get(), path.c_str()), SDL_DestroyTexture);
    if (!texture) {
        throw std::runtime_error("Failed to load " + filename + ": " + SDL_GetError());
    }
    //smooth sprites when scaling down or rotating
    if (!SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_LINEAR)) {
        throw std::runtime_error("Texture filtering failed: " + std::string(SDL_GetError()));
    }
    return texture;
}

void Game::createSceneTexture(bool antiAliasing)
{
    //draw the scene with four samples per game pixel before scaling it down
    const int scale = antiAliasing ? 2 : 1;
    TexturePtr replacement(SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET, windowWidth * scale, windowHeight * scale), SDL_DestroyTexture);
    const bool configured = replacement &&
        SDL_SetTextureScaleMode(replacement.get(), SDL_SCALEMODE_LINEAR) &&
        SDL_SetTextureBlendMode(replacement.get(), SDL_BLENDMODE_NONE) &&
        SDL_SetRenderTarget(renderer.get(), replacement.get()) &&
        SDL_SetRenderScale(renderer.get(), static_cast<float>(scale), static_cast<float>(scale));
    //always return to the window, including when a renderer rejects an option
    const bool restored = SDL_SetRenderTarget(renderer.get(), nullptr);
    if (!configured || !restored) {
        throw std::runtime_error("Anti-aliasing setup failed: " + std::string(SDL_GetError()));
    }
    sceneTexture = std::move(replacement);
}

void Game::loadLevel(const LevelConfig& settings)
{
    //level assets and objects are created here rather than in the entry point
    unloadLevel();
    levelSettings = settings;
    runRewards = settings.runRewards;
    createSceneTexture(options.antiAliasing);
    floorTexture = loadTexture(settings.floorImage);
    launcherTexture = loadTexture(settings.launcherImage);
    loadedLauncherImage = settings.launcherImage;
    ballTexture = loadTexture(settings.ballImage);
    premiumBallTexture = loadTexture(settings.premiumBallImage);
    for (std::size_t i = 0; i < coinTextures.size(); ++i) {
        coinTextures[i] = loadTexture(settings.coinImages[i]);
    }
    coinIconTexture = loadTexture(settings.coinIconImage);
    for (std::size_t i = 0; i < trampolineTextures.size(); ++i) {
        trampolineTextures[i] = loadTexture(settings.trampolineImages[i]);
    }
    mountainTexture = loadTexture(settings.mountainImage);
    cloudTexture = loadTexture(settings.cloudImage);
    background = std::make_unique<Background>(mountainTexture.get(), cloudTexture.get(),
                                               floorY, settings.background);

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = settings.gravity;
    physicsWorld = b2CreateWorld(&worldDef);

    //keep the existing launcher size and position above the floor
    launcher = std::make_unique<Launcher>(
        launcherTexture.get(),
        SDL_FPoint{settings.launcherX, floorY - 366.0f * 0.45f},
        SDL_FPoint{128.0f, 192.0f}, SDL_FPoint{64.0f, 35.0f},
        settings.launchPower, settings.maxDragDistance
    );
    const SDL_FPoint anchor = launcher->getAnchor();
    ball = std::make_unique<Ball>(physicsWorld, ballTexture.get(), anchor, floorY,
                                  pixelsPerMetre, settings.ballHeight, settings.ballSpriteIndex);
    ground = std::make_unique<GroundManager>(physicsWorld, settings.groundFriction,
                                             settings.groundRollingResistance);
    const std::array<SDL_Texture*, 3> coinSprites = {
        coinTextures[0].get(), coinTextures[1].get(), coinTextures[2].get()
    };
    coins = std::make_unique<CoinManager>(coinSprites, floorY, anchor.x + 250.0f,
                                         settings.coinValueWeights);
    const std::array<SDL_Texture*, 3> trampolineSprites = {
        trampolineTextures[0].get(), trampolineTextures[1].get(), trampolineTextures[2].get()
    };
    trampolines = std::make_unique<TrampolineManager>(trampolineSprites, floorY,
        anchor.x + 400.0f, settings.trampolines);
    accumulator = 0.0;
    resetRun();
}

void Game::unloadLevel()
{
    if (renderer) {
        SDL_SetRenderTarget(renderer.get(), nullptr);
    }
    sceneTexture.reset();
    if (sdlReady) {
        SDL_CaptureMouse(false);
    }
    //release objects before the resources they borrow
    ball.reset();
    ground.reset();
    coins.reset();
    trampolines.reset();
    launcher.reset();
    background.reset();
    if (B2_IS_NON_NULL(physicsWorld)) {
        b2DestroyWorld(physicsWorld);
        physicsWorld = b2_nullWorldId;
    }
    for (auto& texture : coinTextures) {
        texture.reset();
    }
    coinIconTexture.reset();
    for (auto& texture : trampolineTextures) {
        texture.reset();
    }
    cloudTexture.reset();
    mountainTexture.reset();
    ballTexture.reset();
    premiumBallTexture.reset();
    launcherTexture.reset();
    floorTexture.reset();
}

void Game::resetRun()
{
    result.reset();
    rewardEffects->clear();
    stoppedTime = 0.0;
    accumulator = 0.0;
    ball->reset();
    sounds->reset();
    impactCooldown = 0.0;
    lastAudioPosition = ball->getPosition();
    applyEquippedSkin();
    //take a snapshot so purchases made while paused only affect the next attempt
    activeUpgrades = upgrades.getEffects();
    boostsRemaining = activeUpgrades.boostCharges;
    ball->setGravityScale(activeUpgrades.gravityScale);
    const std::array<std::string, 6> launcherImages = {
        levelSettings.launcherImage, "game-launcher-level-01.png", "game-launcher-level-02.png",
        "game-launcher-level-03.png", "game-launcher-level-04.png", "game-launcher-level-05-fixed.png"
    };
    const auto& sprite = launcherImages.at(static_cast<std::size_t>(upgrades.getLevel(UpgradeId::Launcher)));
    if (sprite != loadedLauncherImage) {
        launcherTexture = loadTexture(sprite);
        loadedLauncherImage = sprite;
    }
    launcher->configure(launcherTexture.get(), levelSettings.launchPower * activeUpgrades.launchMultiplier,
                        levelSettings.maxDragDistance + activeUpgrades.pullBonus,
                        upgrades.getLevel(UpgradeId::Bands));
    auto coinWeights = levelSettings.coinValueWeights;
    for (std::size_t i = 0; i < coinWeights.size(); ++i) {
        coinWeights[i] *= activeUpgrades.coinWeightMultipliers[i];
    }
    coins->setValueWeights(coinWeights);
    camera.reset();
    SDL_CaptureMouse(false);
    const SDL_FPoint anchor = launcher->getAnchor();
    runStats.reset(anchor.x);
    ground->update(anchor.x / pixelsPerMetre);
    coins->startRun(anchor.x, anchor.y);
    trampolines->startRun(anchor.x, anchor.y);
}

void Game::finishRun()
{
    //freeze and bank the result once, including when the player ends a run early
    if (!hasRun || result) {
        return;
    }
    result = RunResult::calculate(runStats, coins->getRunCoins(), runRewards);
    const auto before = environments;
    environments.recordRun(activeEnvironment, result->distance, result->height);
    std::string unlockMessage;
    for (std::size_t id = 1; id < EnvironmentManager::count; ++id) {
        if (!before.isUnlocked(id) && environments.isUnlocked(id))
            unlockMessage += std::string(EnvironmentManager::definitions[id].name) + " unlocked! ";
    }
    coinBank += result->payout;
    hasRun = false;
    screen = Screen::Results;
    accumulator = 0.0;
    SDL_CaptureMouse(false);
    menuMessage = unlockMessage;
    if (!saveProgress()) {
        menuMessage = "Payout added, but progress and unlocks could not be saved.";
    }
    menu->resetInput();
}

void Game::loadProgress()
{
    if (progressPath.empty()) {
        using PathPtr = std::unique_ptr<char, decltype(&SDL_free)>;
        PathPtr directory(SDL_GetPrefPath("Protolaunch", "Protolaunch"), SDL_free);
        if (!directory) {
            menuMessage = "Progress storage is unavailable.";
            return;
        }
        const std::string directoryText(directory.get());
        const std::u8string utf8Directory(directoryText.begin(), directoryText.end());
        progressPath = std::filesystem::path(utf8Directory) / "progress.dat";
    }
    //a failed load leaves the current progression untouched
    switch (ProgressStore::load(progressPath, upgrades, coinBank, skins, environments)) {
    case ProgressStore::Load::Loaded:
        break;
    case ProgressStore::Load::Missing:
        break;
    case ProgressStore::Load::Invalid:
        menuMessage = "Save could not be read; using default progression.";
        break;
    }
}

bool Game::saveProgress() const
{
    return ProgressStore::save(progressPath, upgrades, coinBank, skins, environments);
}

void SDLCALL Game::progressDialogCallback(void* userdata, const char* const* files, int)
{
    //SDL may call back on another thread, including after the game has closed.
    //The callback owns shared state and never touches Game or its renderer.
    std::unique_ptr<std::shared_ptr<ProgressDialog>> owner(
        static_cast<std::shared_ptr<ProgressDialog>*>(userdata));
    const auto state = *owner;
    std::lock_guard lock(state->mutex);
    state->failed = files == nullptr;
    if (files && files[0]) state->path = files[0];
    if (state->failed) SDL_Log("Save/load dialog failed: %s", SDL_GetError());
    state->ready = true;
}

void Game::openProgressDialog(bool saving)
{
    if (progressDialog) return;
    progressDialog = std::make_shared<ProgressDialog>();
    progressDialog->saving = saving;
    const auto directory = lastManualSave.empty() ? progressPath.parent_path() : lastManualSave.parent_path();
    progressDialog->defaultLocation = pathText(saving ? directory / "my-save.dat" :
                                              (lastManualSave.empty() ? directory : lastManualSave));
    //Filters must outlive the asynchronous native dialog.
    static constexpr SDL_DialogFileFilter filters[] = {{"Protolaunch saves", "dat"}, {"All files", "*"}};
    auto* callbackState = new std::shared_ptr<ProgressDialog>(progressDialog);
    const char* location = progressDialog->defaultLocation.empty() ? nullptr : progressDialog->defaultLocation.c_str();
    menuMessage = saving ? "Choose a name and location for your save." : "Choose a save file to load.";
    if (saving) SDL_ShowSaveFileDialog(progressDialogCallback, callbackState, window.get(), filters, 2, location);
    else SDL_ShowOpenFileDialog(progressDialogCallback, callbackState, window.get(), filters, 2, location, false);
}

void Game::pollProgressDialog()
{
    if (!progressDialog) return;
    bool failed = false;
    bool saving = false;
    std::string selected;
    {
        std::lock_guard lock(progressDialog->mutex);
        if (!progressDialog->ready) return;
        failed = progressDialog->failed;
        saving = progressDialog->saving;
        selected = progressDialog->path;
    }
    progressDialog.reset();
    menu->resetInput();
    if (failed) {
        menuMessage = "File dialog unavailable. Current progress kept.";
        return;
    }
    if (selected.empty()) {
        menuMessage = saving ? "Save cancelled." : "Load cancelled. Current progress kept.";
        return;
    }
    auto path = std::filesystem::path(std::u8string(selected.begin(), selected.end()));
    if (saving) {
        if (path.extension().empty()) {
            path += ".dat";
            //SDL's Windows dialog does not append an extension. If adding one
            //finds an existing file, ask before replacing a file the dialog never checked.
            std::error_code error;
            const bool exists = std::filesystem::exists(path, error);
            if (error) {
                menuMessage = "Could not access save location. Current progress kept.";
                return;
            }
            if (exists) {
                const SDL_MessageBoxButtonData buttons[] = {
                    {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel"},
                    {0, 1, "Replace"}
                };
                const SDL_MessageBoxData box{SDL_MESSAGEBOX_WARNING, window.get(), "Replace save?",
                    "A save with that name already exists. Replace it?", 2, buttons, nullptr};
                int choice = 0;
                if (!SDL_ShowMessageBox(&box, &choice) || choice != 1) {
                    menuMessage = "Save cancelled. Existing file kept.";
                    return;
                }
            }
        }
        if (ProgressStore::save(path, upgrades, coinBank, skins, environments)) {
            lastManualSave = path;
            menuMessage = "Saved: " + pathText(path.filename());
        } else menuMessage = "Could not save file. Current progress kept.";
        return;
    }
    UpgradeManager loadedUpgrades;
    SkinManager loadedSkins;
    EnvironmentManager loadedEnvironments;
    std::int64_t loadedBank = 0;
    if (ProgressStore::load(path, loadedUpgrades, loadedBank, loadedSkins, loadedEnvironments) != ProgressStore::Load::Loaded) {
        menuMessage = "Invalid or unreadable save. Current progress kept.";
        return;
    }
    upgrades = loadedUpgrades;
    skins = loadedSkins;
    environments = loadedEnvironments;
    coinBank = loadedBank;
    lastManualSave = path;
    //Loading a different progression must not carry rewards from the old paused run.
    hasRun = false;
    activeEnvironment = environments.getSelected();
    loadLevel(environments.makeLevel(activeEnvironment));
    menuMessage = saveProgress() ? "Loaded: " + pathText(path.filename()) :
                                  "Save loaded, but autosave failed.";
}

void Game::startFreshGame()
{
    if (progressPath.empty() || !ProgressStore::backup(progressPath.parent_path(), upgrades, coinBank, skins, environments)) {
        screen = Screen::Progress;
        menu->resetInput();
        menuMessage = "Could not back up progress. Fresh game cancelled.";
        return;
    }
    UpgradeManager freshUpgrades;
    SkinManager freshSkins;
    EnvironmentManager freshEnvironments;
    //Keep the current session intact if writing the fresh autosave fails.
    if (!ProgressStore::save(progressPath, freshUpgrades, 0, freshSkins, freshEnvironments)) {
        screen = Screen::Progress;
        menu->resetInput();
        menuMessage = "Could not save fresh game. Current progress kept.";
        return;
    }
    upgrades = freshUpgrades;
    skins = freshSkins;
    environments = freshEnvironments;
    coinBank = 0;
    hasRun = false;
    activeEnvironment = 0;
    loadLevel(environments.makeLevel(activeEnvironment));
    screen = Screen::MainMenu;
    shopReturnScreen = Screen::MainMenu;
    environmentReturnScreen = Screen::MainMenu;
    menu->resetInput();
    menuMessage = "Fresh game ready. Previous progress backed up.";
}

void Game::applyEquippedSkin()
{
    const auto& skin = SkinManager::catalog().at(skins.getEquipped());
    ball->setSkin(skin.sheet == SkinSheet::Standard ? ballTexture.get() : premiumBallTexture.get(), skin.source);
}

void Game::buySkin(std::size_t id)
{
    const auto previousSkins = skins;
    const auto previousBank = coinBank;
    const auto purchase = skins.buyOrEquip(id, coinBank);
    if (purchase == SkinManager::Purchase::NotEnoughCoins) {
        menuMessage = "Earn more coins to unlock this skin.";
    } else if (purchase == SkinManager::Purchase::AlreadyEquipped) {
        menuMessage = "This skin is already equipped.";
    } else if (purchase == SkinManager::Purchase::Invalid) {
        menuMessage = "This skin is unavailable.";
    } else if (!saveProgress()) {
        skins = previousSkins;
        coinBank = previousBank;
        menuMessage = "Skin could not be saved; purchase cancelled.";
    } else {
        applyEquippedSkin();
        menuMessage = std::string(SkinManager::catalog().at(id).name) + " equipped!";
    }
}

void Game::buyUpgrade(UpgradeId id)
{
    const auto previousLevels = upgrades.getLevels();
    const auto previousBank = coinBank;
    const auto purchase = upgrades.buy(id, coinBank);
    if (purchase == UpgradeManager::Purchase::NotEnoughCoins) {
        menuMessage = "Earn more coins to buy this upgrade.";
    } else if (purchase == UpgradeManager::Purchase::MaxLevel) {
        menuMessage = "This upgrade is already at its maximum level.";
    } else if (!saveProgress()) {
        //keep coins and equipment together if the purchase cannot be saved
        upgrades.restoreLevels(previousLevels);
        coinBank = previousBank;
        menuMessage = "Purchase could not be saved; coins refunded.";
    } else {
        menuMessage = std::string(UpgradeManager::definitions.at(static_cast<std::size_t>(id)).name) +
                      " upgraded. Ready for your next run!";
    }
}

void Game::selectEnvironment(std::size_t id)
{
    const auto previous = environments;
    if (!environments.select(id)) {
        menuMessage = "Reach the target in the previous environment first.";
    } else if (!saveProgress()) {
        environments = previous;
        menuMessage = "Environment choice could not be saved. Try again.";
    } else {
        menuMessage = std::string(EnvironmentManager::definitions[id].name) + " selected for your next run.";
    }
}

Menu::View Game::menuView() const
{
    if (screen == Screen::Shop) return Menu::View::Shop;
    if (screen == Screen::Skins) return Menu::View::Skins;
    if (screen == Screen::Results) return Menu::View::Results;
    if (screen == Screen::Options) return Menu::View::Options;
    if (screen == Screen::Environments) return Menu::View::Environments;
    if (screen == Screen::Progress) return Menu::View::Progress;
    if (screen == Screen::FreshGame) return Menu::View::FreshGame;
    return Menu::View::Main;
}

void Game::handleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        SDL_ConvertEventToRenderCoordinates(renderer.get(), &event);
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
            continue;
        }
        if (progressDialog) continue; //the native file dialog owns input until it finishes
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE && !event.key.repeat) {
            if (screen == Screen::Playing) {
                ball->cancelDrag();
                SDL_CaptureMouse(false);
                screen = Screen::MainMenu;
                menu->resetInput();
                menuMessage.clear();
            } else if (screen != Screen::MainMenu) {
                menu->close(Menu::Action::Back);
            } else if (hasRun) {
                menu->close(Menu::Action::Play);
            }
            accumulator = 0.0;
            continue;
        }
        if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST && screen == Screen::Playing) {
            ball->cancelDrag();
            SDL_CaptureMouse(false);
            screen = Screen::MainMenu;
            accumulator = 0.0;
            menu->resetInput();
        }
        //menu events never reach the ball, even when a button starts a run
        if (screen != Screen::Playing) {
            const auto action = menu->handleEvent(event, menuView(), hasRun, options, &upgrades, coinBank, &skins, &environments);
            const auto feedback = menu->takeFeedback();
            if (feedback == Menu::Feedback::Hover) sounds->play(SoundEffects::Effect::ButtonHover, 0.11f);
            if (feedback == Menu::Feedback::Click) sounds->play(SoundEffects::Effect::ButtonClick, 0.24f);
            applyMenuAction(action);
            continue;
        }
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_R && !event.key.repeat) {
            if (ball->getState() == Ball::State::Flying) {
                finishRun();
            } else {
                applyMenuAction(Menu::Action::Restart);
            }
            continue;
        }
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_UP && !event.key.repeat &&
            ball->getState() == Ball::State::Flying && boostsRemaining > 0) {
            ball->boost(450.0f);
            --boostsRemaining;
        }
        const auto previousState = ball->getState();
        launcher->handleEvent(event, camera, *ball);
        if (previousState == Ball::State::Dragging && ball->getState() == Ball::State::Flying) {
            const auto anchor = launcher->getAnchor();
            const auto position = ball->getPosition();
            const float tension = std::clamp(std::hypot(position.x - anchor.x, position.y - anchor.y) /
                (levelSettings.maxDragDistance + activeUpgrades.pullBonus), 0.0f, 1.0f);
            sounds->play(SoundEffects::Effect::Release, 0.35f + tension * 0.40f, 0.85f + tension * 0.25f);
        }
    }
}

void Game::applyMenuAction(Menu::Action action)
{
    if (action == Menu::Action::None) {
        return;
    }
    menuMessage.clear();
    switch (action) {
    case Menu::Action::Play:
    case Menu::Action::Restart:
        if (!hasRun || action == Menu::Action::Restart) {
            if (activeEnvironment != environments.getSelected()) {
                activeEnvironment = environments.getSelected();
                loadLevel(environments.makeLevel(activeEnvironment));
            } else {
                resetRun();
            }
        }
        hasRun = true;
        screen = Screen::Playing;
        accumulator = 0.0;
        break;
    case Menu::Action::Options:
        screen = Screen::Options;
        break;
    case Menu::Action::Environments:
        environmentReturnScreen = screen;
        screen = Screen::Environments;
        menu->resetInput();
        break;
    case Menu::Action::Environment0:
    case Menu::Action::Environment1:
    case Menu::Action::Environment2:
    case Menu::Action::Environment3:
    case Menu::Action::Environment4:
    case Menu::Action::Environment5:
        selectEnvironment(static_cast<std::size_t>(action) - static_cast<std::size_t>(Menu::Action::Environment0));
        break;
    case Menu::Action::Progress:
        screen = Screen::Progress;
        menu->resetInput();
        break;
    case Menu::Action::SaveProgress:
        openProgressDialog(true);
        break;
    case Menu::Action::LoadProgress:
        openProgressDialog(false);
        break;
    case Menu::Action::FreshGame:
        screen = Screen::FreshGame;
        menu->resetInput();
        break;
    case Menu::Action::ConfirmFreshGame:
        startFreshGame();
        break;
    case Menu::Action::Shop:
        if (screen != Screen::Skins && screen != Screen::Shop) shopReturnScreen = screen;
        screen = Screen::Shop;
        menu->resetInput();
        break;
    case Menu::Action::Skins:
        if (screen != Screen::Shop && screen != Screen::Skins) shopReturnScreen = screen;
        screen = Screen::Skins;
        menu->resetInput();
        break;
    case Menu::Action::Skin0:
    case Menu::Action::Skin1:
    case Menu::Action::Skin2:
    case Menu::Action::Skin3:
    case Menu::Action::Skin4:
    case Menu::Action::Skin5:
    case Menu::Action::Skin6:
    case Menu::Action::Skin7:
        buySkin(menu->getSkinId(action));
        break;
    case Menu::Action::StandardSkins:
    case Menu::Action::PremiumSkins:
    case Menu::Action::PreviousSkins:
    case Menu::Action::NextSkins:
        break;
    case Menu::Action::BuyLauncher: buyUpgrade(UpgradeId::Launcher); break;
    case Menu::Action::BuyBands: buyUpgrade(UpgradeId::Bands); break;
    case Menu::Action::BuyMagnet: buyUpgrade(UpgradeId::Magnet); break;
    case Menu::Action::BuyLoot: buyUpgrade(UpgradeId::LuckyLoot); break;
    case Menu::Action::BuyGlider: buyUpgrade(UpgradeId::Glider); break;
    case Menu::Action::BuyAirBoost: buyUpgrade(UpgradeId::AirBoost); break;
    case Menu::Action::EndRun:
        finishRun();
        break;
    case Menu::Action::MainMenu:
        screen = Screen::MainMenu;
        break;
    case Menu::Action::Back:
        screen = screen == Screen::FreshGame ? Screen::Progress :
            (screen == Screen::Environments ? environmentReturnScreen :
            (screen == Screen::Shop || screen == Screen::Skins ? shopReturnScreen : Screen::MainMenu));
        //The fresh-game confirmation returns to Save / Load without changing progress.
        break;
    case Menu::Action::Quit:
        running = false;
        break;
    case Menu::Action::Fullscreen:
        if (SDL_SetWindowFullscreen(window.get(), !options.fullscreen)) {
            options.fullscreen = !options.fullscreen;
        } else {
            menuMessage = "Fullscreen is unavailable on this display.";
        }
        break;
    case Menu::Action::VSync:
        if (SDL_SetRenderVSync(renderer.get(), options.vsync ? 0 : 1)) {
            options.vsync = !options.vsync;
        } else {
            menuMessage = "VSync is unavailable with this renderer.";
        }
        break;
    case Menu::Action::AntiAliasing:
        try {
            createSceneTexture(!options.antiAliasing);
            options.antiAliasing = !options.antiAliasing;
        } catch (const std::runtime_error&) {
            menuMessage = "This graphics setting could not be applied.";
        }
        break;
    case Menu::Action::MusicDown:
        options.musicVolume = std::max(0, options.musicVolume - 10);
        break;
    case Menu::Action::MusicUp:
        options.musicVolume = std::min(100, options.musicVolume + 10);
        break;
    case Menu::Action::EffectsDown:
        options.effectsVolume = std::max(0, options.effectsVolume - 10);
        break;
    case Menu::Action::EffectsUp:
        options.effectsVolume = std::min(100, options.effectsVolume + 10);
        break;
    case Menu::Action::None:
        break;
    }
    //keep selection on toggles so keyboard users can change them repeatedly
    if (action == Menu::Action::Play || action == Menu::Action::Restart ||
        action == Menu::Action::Options || action == Menu::Action::Back ||
        action == Menu::Action::MainMenu) {
        menu->resetInput();
    }
}

void Game::update(double elapsed)
{
    pollProgressDialog();
    if (screen != Screen::Playing) {
        applyMenuAction(menu->update(elapsed));
        accumulator = 0.0;
        return;
    }
    //limit catch up after a pause and keep physics at 60 steps per second
    accumulator += std::min(elapsed, 0.25);
    while (accumulator >= physicsStep) {
        rewardEffects->update(static_cast<float>(physicsStep));
        trampolines->animate(static_cast<float>(physicsStep));
        const SDL_FPoint previous = ball->getPosition();
        impactCooldown = std::max(0.0, impactCooldown - physicsStep);
        ground->update(ball->getPhysicsPosition().x);
        b2World_Step(physicsWorld, static_cast<float>(physicsStep), 4);
        ball->updateFromPhysics();

        if (ball->getState() == Ball::State::Flying) {
            //The ground is the only physical collider; trampoline crossings are handled below.
            //Use collision events so fast landings still sound if the ball has already rebounded.
            const auto contacts = b2World_GetContactEvents(physicsWorld);
            float impactSpeed = 0.0f;
            for (int i = 0; i < contacts.hitCount; ++i) {
                impactSpeed = std::max(impactSpeed, contacts.hitEvents[i].approachSpeed);
            }
            if (impactSpeed > 0.8f && impactCooldown <= 0.0) {
                const float strength = std::clamp(impactSpeed / 20.0f, 0.0f, 1.0f);
                sounds->play(SoundEffects::Effect::Impact, 0.18f + strength * 0.60f,
                             1.10f - strength * 0.25f);
                impactCooldown = 0.10;
            }
            //check the whole movement so fast falls cannot skip through a mat
            const SDL_FPoint current = ball->getPosition();
            trampolines->update(current.x, current.y);
            if (const auto landing = trampolines->land(previous, current, ball->getRadius())) {
                ball->bounce(landing->position, landing->upwardSpeed);
                sounds->play(SoundEffects::Effect::Trampoline, 0.65f,
                             0.95f + static_cast<float>(landing->reward) * 0.006f);
                coins->addCoins(landing->reward);
                rewardEffects->add(camera.worldToScreen(landing->position.x, landing->position.y),
                                   landing->reward, RewardEffects::Kind::Trampoline);
            }
            const SDL_FPoint position = ball->getPosition();
            runStats.update(position, floorY, ball->getRadius(), physicsStep);
            coins->update(position.x, position.y);
            coins->collect(position.x, position.y, ball->getRadius(), activeUpgrades.magnetRange);
            for (const auto& pickup : coins->getPickups()) {
                rewardEffects->add(camera.worldToScreen(pickup.position.x, pickup.position.y),
                                   pickup.value, RewardEffects::Kind::Coin);
            }
            //One chime per pickup batch keeps dense coin clusters from becoming harsh.
            if (!coins->getPickups().empty()) {
                int value = 1;
                for (const auto& pickup : coins->getPickups()) value = std::max(value, pickup.value);
                sounds->play(SoundEffects::Effect::Coin, 0.42f, 1.0f + static_cast<float>(value - 1) * 0.09f);
            }
            //require a short, continuous rest so ordinary landings do not end the run
            const b2Vec2 velocity = ball->getPhysicsVelocity();
            const bool onGround = position.y + ball->getRadius() >= floorY - 2.0f;
            const bool stopped = std::abs(velocity.x) < 0.15f && std::abs(velocity.y) < 0.15f;
            stoppedTime = onGround && stopped ? stoppedTime + physicsStep : 0.0;
            if (stoppedTime >= 0.75) {
                finishRun();
                break;
            }
        }
        accumulator -= physicsStep;
    }

    if (ball->getState() == Ball::State::Flying) {
        const SDL_FPoint position = ball->getPosition();
        camera.follow(position.x, position.y);
    }
}

void Game::updateAudio(double elapsed)
{
    sounds->setVolumes(static_cast<float>(options.musicVolume) / 100.0f,
                       static_cast<float>(options.effectsVolume) / 100.0f);
    sounds->setMusic(!running ? SoundEffects::Music::None :
                     (screen == Screen::Playing && ball->getState() == Ball::State::Flying
                         ? SoundEffects::Music::Gameplay : SoundEffects::Music::Menu));
    sounds->setLoop(SoundEffects::Effect::Pull, 0.0f);
    sounds->setLoop(SoundEffects::Effect::Roll, 0.0f);
    const auto position = ball->getPosition();
    if (screen == Screen::Playing && ball->getState() == Ball::State::Dragging) {
        const auto anchor = launcher->getAnchor();
        const float tension = std::clamp(std::hypot(position.x - anchor.x, position.y - anchor.y) /
            (levelSettings.maxDragDistance + activeUpgrades.pullBonus), 0.0f, 1.0f);
        const float movement = std::hypot(position.x - lastAudioPosition.x, position.y - lastAudioPosition.y) /
                               static_cast<float>(std::max(elapsed, 0.001));
        sounds->setLoop(SoundEffects::Effect::Pull,
                        tension * (0.025f + 0.28f * std::clamp(movement / 350.0f, 0.0f, 1.0f)),
                        0.70f + 0.80f * tension);
    } else if (screen == Screen::Playing && ball->getState() == Ball::State::Flying &&
               position.y + ball->getRadius() >= floorY - 2.0f) {
        const auto velocity = ball->getPhysicsVelocity();
        const float speed = std::abs(velocity.x);
        if (speed > 0.2f && std::abs(velocity.y) < 0.8f) {
            const float amount = std::clamp(speed / 15.0f, 0.0f, 1.0f);
            sounds->setLoop(SoundEffects::Effect::Roll, 0.014f + amount * 0.08f, 0.65f + amount * 0.80f);
        }
    }
    lastAudioPosition = position;
    sounds->update();
}

void Game::draw() const
{
    //the scene target remembers its scale separately from the window
    if (!SDL_SetRenderTarget(renderer.get(), sceneTexture.get())) {
        throw std::runtime_error("Scene rendering failed: " + std::string(SDL_GetError()));
    }
    //background layers move more slowly than the foreground
    background->draw(renderer.get(), camera, ball->getPosition().y);

    //repeat the floor relative to the camera
    float offset = std::fmod(camera.getX(), tileWidth);
    if (offset < 0.0f) {
        offset += tileWidth;
    }
    for (float x = -offset; x < windowWidth; x += tileWidth) {
        SDL_FRect destination = {x, floorY - camera.getY(), tileWidth, tileHeight};
        SDL_RenderTexture(renderer.get(), floorTexture.get(), nullptr, &destination);
    }

    coins->drawPickupRange(renderer.get(), camera, ball->getPosition(), ball->getRadius(),
                           activeUpgrades.magnetRange);
    coins->draw(renderer.get(), camera);
    trampolines->draw(renderer.get(), camera);
    launcher->draw(renderer.get(), camera, *ball);
    ball->draw(renderer.get(), camera);

    //downsample the scene, then draw the interface at the window resolution
    if (!SDL_SetRenderTarget(renderer.get(), nullptr)) {
        throw std::runtime_error("Window rendering failed: " + std::string(SDL_GetError()));
    }
    SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
    SDL_RenderClear(renderer.get());
    if (!SDL_RenderTexture(renderer.get(), sceneTexture.get(), nullptr, nullptr)) {
        throw std::runtime_error("Scene presentation failed: " + std::string(SDL_GetError()));
    }

    rewardEffects->draw();
    if (screen != Screen::Playing) {
        menu->draw(menuView(), hasRun, options, menuMessage, result ? &*result : nullptr, coinBank, &upgrades, &skins, &environments);
        SDL_RenderPresent(renderer.get());
        return;
    }

    //the plain gold coin represents the total value in the interface
    const SDL_FRect coinIcon = {16.0f, 14.0f, 32.0f, 32.0f};
    SDL_RenderTexture(renderer.get(), coinIconTexture.get(), nullptr, &coinIcon);
    const std::string coinText = std::to_string(coins->getRunCoins());
    //use a shadow so the counter stays readable against bright and dark skies
    SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
    SDL_RenderDebugText(renderer.get(), 57.0f, 27.0f, coinText.c_str());
    SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);
    SDL_RenderDebugText(renderer.get(), 56.0f, 26.0f, coinText.c_str());
    const std::string bankText = "Bank: " + std::to_string(coinBank);
    SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
    SDL_RenderDebugText(renderer.get(), 17.0f, 55.0f, bankText.c_str());
    SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);
    SDL_RenderDebugText(renderer.get(), 16.0f, 54.0f, bankText.c_str());
    const std::string boostText = "Boosts: " + std::to_string(boostsRemaining) + " [Up]";
    SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
    SDL_RenderDebugText(renderer.get(), 17.0f, 79.0f, boostText.c_str());
    SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);
    SDL_RenderDebugText(renderer.get(), 16.0f, 78.0f, boostText.c_str());
    const std::string areaText = std::string(EnvironmentManager::definitions[activeEnvironment].name);
    SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
    SDL_RenderDebugText(renderer.get(), 17.0f, 103.0f, areaText.c_str());
    SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);
    SDL_RenderDebugText(renderer.get(), 16.0f, 102.0f, areaText.c_str());

    //keep the run measurements fixed on screen as the camera follows the ball
    const SDL_FRect statsPanel{windowWidth - 240.0f, 14.0f, 224.0f, 112.0f};
    SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer.get(), 15, 30, 45, 175);
    SDL_RenderFillRect(renderer.get(), &statsPanel);
    SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_NONE);
    const auto statLine = [&](const char* label, double value, float y, const char* unit = " m") {
        std::ostringstream text;
        text << label << std::fixed << std::setprecision(1) << value << unit;
        const std::string line = text.str();
        SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
        SDL_RenderDebugText(renderer.get(), statsPanel.x + 13, y + 1, line.c_str());
        SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);
        SDL_RenderDebugText(renderer.get(), statsPanel.x + 12, y, line.c_str());
    };
    statLine("Distance: ", runStats.getDistance(), 30);
    statLine("Height: ", runStats.getHeight(), 54);
    statLine("Max height: ", runStats.getBestHeight(), 78);
    statLine("Run time: ", runStats.getDuration(), 102, " s");
    SDL_RenderPresent(renderer.get());
}

int Game::run()
{
    initialize();
    loadProgress();
    activeEnvironment = environments.getSelected();
    loadLevel(environments.makeLevel(activeEnvironment));

    Uint64 previousTicks = SDL_GetTicks();
    running = true;
    while (running) {
        handleEvents();
        const Uint64 currentTicks = SDL_GetTicks();
        const double elapsed = static_cast<double>(currentTicks - previousTicks) / 1000.0;
        previousTicks = currentTicks;

        update(elapsed);
        updateAudio(elapsed);
        draw();
        //vsync already waits for the display, so avoid waiting a second frame
        SDL_Delay(options.vsync ? 1 : 16);
    }
    //Let the final music fade reach the device before SDL tears down the stream.
    sounds->reset();
    sounds->setMusic(SoundEffects::Music::None);
    const Uint64 fadeDeadline = SDL_GetTicks() + 1100;
    while (sounds->isMusicPlaying() && SDL_GetTicks() < fadeDeadline) {
        sounds->update();
        SDL_Delay(8);
    }
    SDL_Delay(50); //drain the mixer's final queued audio
    return 0;
}
