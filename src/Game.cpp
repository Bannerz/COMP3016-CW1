#include "Game.h"
#include "Ball.h"
#include "Launcher.h"
#include "CoinManager.h"
#include "GroundManager.h"
#include "Background.h"
#include "TrampolineManager.h"

#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

Game::Game() = default;

Game::~Game()
{
    //destroy level objects before their world and textures
    unloadLevel();
    renderer.reset();
    window.reset();
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

    SDL_Window* newWindow = nullptr;
    SDL_Renderer* newRenderer = nullptr;
    const bool created = SDL_CreateWindowAndRenderer(
        "COMP3016", windowWidth, windowHeight, SDL_WINDOW_HIGH_PIXEL_DENSITY,
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

void Game::loadLevel(const LevelConfig& settings)
{
    //level assets and objects are created here rather than in the entry point
    unloadLevel();
    //draw the scene with four samples per game pixel before scaling it down
    sceneTexture.reset(SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET, windowWidth * antiAliasingScale,
        windowHeight * antiAliasingScale));
    if (!sceneTexture ||
        !SDL_SetTextureScaleMode(sceneTexture.get(), SDL_SCALEMODE_LINEAR) ||
        !SDL_SetTextureBlendMode(sceneTexture.get(), SDL_BLENDMODE_NONE) ||
        !SDL_SetRenderTarget(renderer.get(), sceneTexture.get()) ||
        !SDL_SetRenderScale(renderer.get(), antiAliasingScale, antiAliasingScale) ||
        !SDL_SetRenderTarget(renderer.get(), nullptr)) {
        throw std::runtime_error("Anti-aliasing setup failed: " + std::string(SDL_GetError()));
    }
    floorTexture = loadTexture(settings.floorImage);
    launcherTexture = loadTexture(settings.launcherImage);
    ballTexture = loadTexture(settings.ballImage);
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
    launcherTexture.reset();
    floorTexture.reset();
}

void Game::resetRun()
{
    ball->reset();
    camera.reset();
    SDL_CaptureMouse(false);
    const SDL_FPoint anchor = launcher->getAnchor();
    ground->update(anchor.x / pixelsPerMetre);
    coins->startRun(anchor.x, anchor.y);
    trampolines->startRun(anchor.x, anchor.y);
}

void Game::handleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        SDL_ConvertEventToRenderCoordinates(renderer.get(), &event);
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_R && !event.key.repeat) {
            resetRun();
        }
        launcher->handleEvent(event, camera, *ball);
    }
}

void Game::update(double elapsed)
{
    //limit catch up after a pause and keep physics at 60 steps per second
    accumulator += std::min(elapsed, 0.25);
    while (accumulator >= physicsStep) {
        trampolines->animate(static_cast<float>(physicsStep));
        const SDL_FPoint previous = ball->getPosition();
        ground->update(ball->getPhysicsPosition().x);
        b2World_Step(physicsWorld, static_cast<float>(physicsStep), 4);
        ball->updateFromPhysics();

        if (ball->getState() == Ball::State::Flying) {
            //check the whole movement so fast falls cannot skip through a mat
            const SDL_FPoint current = ball->getPosition();
            trampolines->update(current.x, current.y);
            if (const auto landing = trampolines->land(previous, current, ball->getRadius())) {
                ball->bounce(landing->position, landing->upwardSpeed);
            }
            const SDL_FPoint position = ball->getPosition();
            coins->update(position.x, position.y);
            coins->collect(position.x, position.y, ball->getRadius());
        }
        accumulator -= physicsStep;
    }

    if (ball->getState() == Ball::State::Flying) {
        const SDL_FPoint position = ball->getPosition();
        camera.follow(position.x, position.y);
    }
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

    coins->draw(renderer.get(), camera);
    trampolines->draw(renderer.get(), camera);
    launcher->draw(renderer.get(), camera);
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

    //the plain gold coin represents the total value in the interface
    const SDL_FRect coinIcon = {16.0f, 14.0f, 32.0f, 32.0f};
    SDL_RenderTexture(renderer.get(), coinIconTexture.get(), nullptr, &coinIcon);
    const std::string coinText = std::to_string(coins->getRunCoins());
    //use a shadow so the counter stays readable against bright and dark skies
    SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
    SDL_RenderDebugText(renderer.get(), 57.0f, 27.0f, coinText.c_str());
    SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);
    SDL_RenderDebugText(renderer.get(), 56.0f, 26.0f, coinText.c_str());
    const std::string scoreText = "Score: " +
        std::to_string(coins->getRunCoins() + trampolines->getRunScore());
    SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
    SDL_RenderDebugText(renderer.get(), 17.0f, 55.0f, scoreText.c_str());
    SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);
    SDL_RenderDebugText(renderer.get(), 16.0f, 54.0f, scoreText.c_str());
    SDL_RenderPresent(renderer.get());
}

int Game::run()
{
    initialize();
    loadLevel(LevelConfig{});

    Uint64 previousTicks = SDL_GetTicks();
    running = true;
    while (running) {
        handleEvents();
        const Uint64 currentTicks = SDL_GetTicks();
        const double elapsed = static_cast<double>(currentTicks - previousTicks) / 1000.0;
        previousTicks = currentTicks;

        update(elapsed);
        draw();
        SDL_Delay(16);
    }
    return 0;
}
