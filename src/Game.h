#pragma once

#include "Camera.h"
#include "LevelConfig.h"

#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include <array>
#include <memory>
#include <string>

class Ball;
class Launcher;
class CoinManager;
class GroundManager;
class Background;
class TrampolineManager;

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
    void loadLevel(const LevelConfig& settings);
    void unloadLevel();
    TexturePtr loadTexture(const std::string& filename);
    void resetRun();
    void handleEvents();
    void update(double elapsed);
    void draw() const;

    static constexpr int windowWidth = 960;
    static constexpr int windowHeight = 640;
    static constexpr int antiAliasingScale = 2;
    static constexpr float tileWidth = 256.0f;
    static constexpr float tileHeight = 128.0f;
    static constexpr float floorY = windowHeight - tileHeight;
    static constexpr float pixelsPerMetre = 40.0f;
    static constexpr double physicsStep = 1.0 / 60.0;

    bool sdlReady = false;
    bool running = false;
    double accumulator = 0.0;
    std::string assetDirectory;

    WindowPtr window{nullptr, SDL_DestroyWindow};
    RendererPtr renderer{nullptr, SDL_DestroyRenderer};
    TexturePtr sceneTexture{nullptr, SDL_DestroyTexture};
    TexturePtr floorTexture{nullptr, SDL_DestroyTexture};
    TexturePtr launcherTexture{nullptr, SDL_DestroyTexture};
    TexturePtr ballTexture{nullptr, SDL_DestroyTexture};
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
};
