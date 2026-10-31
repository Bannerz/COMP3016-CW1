#pragma once

#include "Camera.h"
#include "LevelConfig.h"
#include "Menu.h"
#include "RunStats.h"
#include "UpgradeManager.h"

#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include <array>
#include <memory>
#include <filesystem>
#include <optional>
#include <string>

class Ball;
class Launcher;
class CoinManager;
class GroundManager;
class Background;
class TrampolineManager;
class RewardEffects;

//own the application resources and coordinate the current level
class Game {
public:
    Game();
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    int run();

private:
    //smart pointers use SDLs cleanup functions when released
    using TexturePtr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
    using RendererPtr = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
    using WindowPtr = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;

    void initialize();
    void createSceneTexture(bool antiAliasing);
    void applyMenuAction(Menu::Action action);
    void buyUpgrade(UpgradeId id);
    void buySkin(std::size_t id);
    void selectEnvironment(std::size_t id);
    void applyEquippedSkin();
    void loadProgress(bool manual = false);
    bool saveProgress() const;
    Menu::View menuView() const;
    void loadLevel(const LevelConfig& settings);
    void unloadLevel();
    TexturePtr loadTexture(const std::string& filename);
    void resetRun();
    void finishRun();
    void handleEvents();
    void update(double elapsed);
    void draw() const;

    static constexpr int windowWidth = 960;
    static constexpr int windowHeight = 640;
    static constexpr float tileWidth = 256.0f;
    static constexpr float tileHeight = 128.0f;
    static constexpr float floorY = windowHeight - tileHeight;
    static constexpr float pixelsPerMetre = 40.0f;
    static constexpr double physicsStep = 1.0 / 60.0;

    bool sdlReady = false;
    bool fontsReady = false;
    bool running = false;
    double accumulator = 0.0;
    std::string assetDirectory;
    enum class Screen { MainMenu, Options, Playing, Results, Shop, Skins, Progress, Environments };
    Screen screen = Screen::MainMenu;
    Screen shopReturnScreen = Screen::MainMenu;
    bool hasRun = false;
    MenuOptions options;
    std::string menuMessage;
    RunStats runStats{pixelsPerMetre};
    RunRewardSettings runRewards;
    std::optional<RunResult> result;
    std::int64_t coinBank = 0;
    double stoppedTime = 0.0;
    UpgradeManager upgrades;
    SkinManager skins;
    EnvironmentManager environments;
    std::size_t activeEnvironment = 0;
    Screen environmentReturnScreen = Screen::MainMenu;
    UpgradeEffects activeUpgrades;
    int boostsRemaining = 0;
    LevelConfig levelSettings;
    std::string loadedLauncherImage;
    std::filesystem::path progressPath;

    WindowPtr window{nullptr, SDL_DestroyWindow};
    RendererPtr renderer{nullptr, SDL_DestroyRenderer};
    TexturePtr sceneTexture{nullptr, SDL_DestroyTexture};
    TexturePtr floorTexture{nullptr, SDL_DestroyTexture};
    TexturePtr launcherTexture{nullptr, SDL_DestroyTexture};
    TexturePtr ballTexture{nullptr, SDL_DestroyTexture};
    TexturePtr premiumBallTexture{nullptr, SDL_DestroyTexture};
    std::array<TexturePtr, 3> coinTextures = {
        TexturePtr{nullptr, SDL_DestroyTexture},
        TexturePtr{nullptr, SDL_DestroyTexture},
        TexturePtr{nullptr, SDL_DestroyTexture}
    };
    TexturePtr coinIconTexture{nullptr, SDL_DestroyTexture};
    std::array<TexturePtr, 3> trampolineTextures = {
        TexturePtr{nullptr, SDL_DestroyTexture},
        TexturePtr{nullptr, SDL_DestroyTexture},
        TexturePtr{nullptr, SDL_DestroyTexture}
    };
    TexturePtr mountainTexture{nullptr, SDL_DestroyTexture};
    TexturePtr cloudTexture{nullptr, SDL_DestroyTexture};

    b2WorldId physicsWorld = b2_nullWorldId;
    Camera camera;
    std::unique_ptr<Ball> ball;
    std::unique_ptr<Launcher> launcher;
    std::unique_ptr<CoinManager> coins;
    std::unique_ptr<GroundManager> ground;
    std::unique_ptr<Background> background;
    std::unique_ptr<TrampolineManager> trampolines;
    std::unique_ptr<Menu> menu;
    std::unique_ptr<RewardEffects> rewardEffects;
};
