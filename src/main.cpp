//Alex Banfield - 2024-06-05 - COMP3016 - Coursework 1

#include <SDL3/SDL.h>
//SDL supplies the entry point for macOS and Windows
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include <string>
#include "BallSprites.h"
#include "Camera.h"
#include "CoinManager.h"
#include "GroundManager.h"
#include <box2d/box2d.h>
#include <algorithm>

//keep track of whether the ball is waiting, being dragged or flying
enum class BallState {
    Ready,
    Dragging,
    Flying
};

int main(int, char*[])
{
    //start the video system before creating a window.
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    //create window and renderer
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("COMP3016", 960, 640, 0,
                                    &window, &renderer)) {
        SDL_Log("Window/renderer creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    //set up game objects and assets after creating the renderer

    //use a fixed drawing area
    SDL_SetRenderLogicalPresentation(
        renderer, 960, 640, SDL_LOGICAL_PRESENTATION_LETTERBOX
    );

    //find assets directory relative to the executable
    const char* basePath = SDL_GetBasePath();
    if (!basePath) {
        SDL_Log("Failed to get base path: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    const std::string floorPath =
        std::string(basePath) + "assets/images/game-ground-yard.png"; //floor sprite (level 1)

    //load image once, ready to redraw each frame
    SDL_Texture* floorTexture = IMG_LoadTexture(renderer, floorPath.c_str());

    if (!floorTexture) {
        SDL_Log("Failed to load floor texture: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    //load the first launcher and ball sheet once during setup
    const std::string launcherPath =
        std::string(basePath) + "assets/images/game-launcher-level-00.png";
    SDL_Texture* launcherTexture = IMG_LoadTexture(renderer, launcherPath.c_str());
    if (!launcherTexture) {
        SDL_Log("Failed to load launcher: %s", SDL_GetError());
        SDL_DestroyTexture(floorTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    const std::string ballSheetPath =
        std::string(basePath) + "assets/images/ball-sprite-sheet.png";
    SDL_Texture* ballTexture = IMG_LoadTexture(renderer, ballSheetPath.c_str());
    if (!ballTexture) {
        SDL_Log("Failed to load ball sheet: %s", SDL_GetError());
        SDL_DestroyTexture(launcherTexture);
        SDL_DestroyTexture(floorTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    //load the coin sprite once during setup
    const std::string coinPath =
        std::string(basePath) + "assets/images/game-coin.png";

    SDL_Texture* coinTexture = IMG_LoadTexture(renderer, coinPath.c_str());

    if (!coinTexture) {
        SDL_Log("Failed to load coin texture: %s", SDL_GetError());
        SDL_DestroyTexture(ballTexture);
        SDL_DestroyTexture(launcherTexture);
        SDL_DestroyTexture(floorTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    //draw the floor texture to the renderer
    const float tileWidth = 256.0f;
    const float tileHeight = 128.0f;

    const float floorY = 640.0f - tileHeight; //position floor at bottom of window

    //draw launcher at half size
    const float launcherX = 140.0f;
    const float launcherY = floorY - 366.0f * 0.45f;
    const float launcherWidth = 128.0f;
    const float launcherHeight = 192.0f;

    //set this to a sprite ID from 0 to 39 to change the ball
    //BallSprites.h stores the source rectangle for each design
    std::size_t ballSpriteIndex = 6;
    const float ballHeight = 44.0f;

    //ball position in world pixels
    float ballX = launcherX + launcherWidth * 0.5f;
    float ballY = launcherY + 35.0f;

    //handle both camera offsets in one place
    Camera camera;

    //the ball returns to this position between attempts
    const float anchorX = ballX;
    const float anchorY = ballY;

    const float ballRadius = ballHeight * 0.5f;

    //maximum distance the ball can be dragged from the anchor point
    const float maxDragDistance = 100.0f;

    //each pixel of pull produces this many pixels/second of launch speed
    const float launchPower = 80.0f;

    //main owns the coin texture, while the manager handles the coin data
    CoinManager coins(coinTexture, floorY, anchorX + 250.0f);
    coins.startRun(anchorX, anchorY);

    BallState ballState = BallState::Ready;

    const float pixelsPerMetre = 40.0f;

    //create the physics world
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, -9.8f};

    b2WorldId physicsWorld = b2CreateWorld(&worldDef);

    //keep the ground manager inside this scope so it is destroyed before the world
    {
        GroundManager ground(physicsWorld);

        //create the ball with physics disabled until launched
        b2BodyDef ballDef = b2DefaultBodyDef();
        ballDef.type = b2_dynamicBody;
        ballDef.position = {
            ballX / pixelsPerMetre,
            (floorY - ballY) / pixelsPerMetre
        };
        ballDef.isEnabled = false;

        //keep the sprite upright
        ballDef.fixedRotation = true;

        b2BodyId ballBody = b2CreateBody(physicsWorld, &ballDef);

        b2Circle ballCircle = {};
        ballCircle.radius = ballRadius / pixelsPerMetre;

        b2ShapeDef ballShapeDef = b2DefaultShapeDef();
        ballShapeDef.density = 1.0f;
        ballShapeDef.material.friction = 0.4f;
        ballShapeDef.material.restitution = 0.35f;

        b2CreateCircleShape(ballBody, &ballShapeDef, &ballCircle);

        //create the starting floor sections
        ground.update(ballX / pixelsPerMetre);

        auto moveDraggedBall = [&](float mouseX, float mouseY) {
            //convert the mouse position into world coordinates
            const SDL_FPoint mouseWorld = camera.screenToWorld(mouseX, mouseY);
            float pullX = mouseWorld.x - anchorX;
            float pullY = mouseWorld.y - anchorY;

            const float distance =
                std::sqrt(pullX * pullX + pullY * pullY);

            //limit how far the ball can be pulled from the launcher
            if (distance > maxDragDistance) {
                const float scale = maxDragDistance / distance;
                pullX *= scale;
                pullY *= scale;
            }

            ballX = anchorX + pullX;
            ballY = anchorY + pullY;
        };

        Uint64 previousTicks = SDL_GetTicks();
        double accumulator = 0.0;

        constexpr double physicsStep = 1.0 / 60.0;

        //main loop
        bool running = true;
        while (running) {
            //process all pending window, keyboard, and mouse events.
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                SDL_ConvertEventToRenderCoordinates(renderer, &event);

                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }

                //handle dragging, launching and resetting the ball

                //start dragging only when the player clicks the ball
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                    event.button.button == SDL_BUTTON_LEFT &&
                    ballState == BallState::Ready) {

                    const SDL_FPoint mouseWorld =
                        camera.screenToWorld(event.button.x, event.button.y);

                    const float dx = mouseWorld.x - ballX;
                    const float dy = mouseWorld.y - ballY;

                    if (dx * dx + dy * dy <= ballRadius * ballRadius) {
                        ballState = BallState::Dragging;
                        SDL_CaptureMouse(true);
                    }
                }

                //move the ball with the mouse while dragging
                if (event.type == SDL_EVENT_MOUSE_MOTION &&
                    ballState == BallState::Dragging) {

                    moveDraggedBall(event.motion.x, event.motion.y);
                }

                if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                    event.button.button == SDL_BUTTON_LEFT &&
                    ballState == BallState::Dragging) {

                    //use the final mouse position when released
                    moveDraggedBall(event.button.x, event.button.y);

                    SDL_CaptureMouse(false);

                    //move the physics body to where the ball was dragged
                    b2Body_SetTransform(
                        ballBody,
                        {
                            ballX / pixelsPerMetre,
                            (floorY - ballY) / pixelsPerMetre
                        },
                        b2Rot_identity
                    );

                    b2Body_Enable(ballBody);

                    //launch in the opposite direction to the pull
                    const b2Vec2 launchVelocity = {
                        (anchorX - ballX) * launchPower / pixelsPerMetre,
                        (ballY - anchorY) * launchPower / pixelsPerMetre
                    };

                    b2Body_SetLinearVelocity(ballBody, launchVelocity);
                    b2Body_SetAwake(ballBody, true);

                    ballState = BallState::Flying;
                }

                //press R to return the ball to the launcher
                if (event.type == SDL_EVENT_KEY_DOWN &&
                    event.key.key == SDLK_R &&
                    !event.key.repeat) {

                    b2Body_SetLinearVelocity(ballBody, {0.0f, 0.0f});
                    b2Body_Disable(ballBody);

                    ballX = anchorX;
                    ballY = anchorY;
                    camera.reset();

                    ballState = BallState::Ready;
                    SDL_CaptureMouse(false);

                    //generate a new layout of coins for the next attempt
                    coins.startRun(anchorX, anchorY);
                }

                //cancel dragging if the player switches away from the window
                if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST &&
                    ballState == BallState::Dragging) {

                    ballX = anchorX;
                    ballY = anchorY;
                    ballState = BallState::Ready;

                    SDL_CaptureMouse(false);
                }

            }

            //update physics at a fixed timestep
            const Uint64 currentTicks = SDL_GetTicks();

            const double elapsed =
                static_cast<double>(currentTicks - previousTicks) / 1000.0;

            previousTicks = currentTicks;

            //limit the time added after a pause or breakpoint
            accumulator += std::min(elapsed, 0.25);

            while (accumulator >= physicsStep) {

                //make sure the floor exists before simulating movement
                const b2Vec2 ballPosition = b2Body_GetPosition(ballBody);
                ground.update(ballPosition.x);

                b2World_Step(
                    physicsWorld,
                    static_cast<float>(physicsStep),
                    4
                );

                //check for collisions between the ball and coins
                if (ballState == BallState::Flying) {
                    const b2Vec2 physicsPosition = b2Body_GetPosition(ballBody);

                    const float physicsBallX = physicsPosition.x * pixelsPerMetre;
                    const float physicsBallY = floorY - physicsPosition.y * pixelsPerMetre;

                    //load nearby sections before checking for pickups
                    coins.update(physicsBallX, physicsBallY);
                    coins.collect(physicsBallX, physicsBallY, ballRadius);
                }

                accumulator -= physicsStep;
            }

            //use the physics position to draw the ball after launching
            if (ballState == BallState::Flying) {
                const b2Vec2 position = b2Body_GetPosition(ballBody);

                ballX = position.x * pixelsPerMetre;
                ballY = floorY - position.y * pixelsPerMetre;
                camera.follow(ballX, ballY);
            }

            //clear the previous frame with a blue background
            SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
            SDL_RenderClear(renderer);

            //draw shapes, sprites, and text after clearing.

            //keep offset within one tile
            float offset = std::fmod(camera.getX(), tileWidth);

            if (offset < 0.0f) {
                offset += tileWidth;
            }

            //draw enough copies to cover the width
            for (float x = -offset; x < 960.0f; x += tileWidth) {
                SDL_FRect destination = {
                    x,
                    floorY - camera.getY(),
                    tileWidth,
                    tileHeight
                };

                SDL_RenderTexture(renderer, floorTexture, nullptr, &destination);
            }

            //draw coins using the same camera as the rest of the world
            coins.draw(renderer, camera);

            //draw the launcher relative to the camera
            const SDL_FPoint launcherScreen = camera.worldToScreen(launcherX, launcherY);
            SDL_FRect launcherDestination = {
                launcherScreen.x, launcherScreen.y, launcherWidth, launcherHeight
            };
            SDL_RenderTexture(renderer, launcherTexture, nullptr, &launcherDestination);

            //select a ball from the sheet and keep its aspect ratio
            //ballX and ballY are the centre of the ball
            const SDL_FRect& ballSource = ballSprites.at(ballSpriteIndex);
            const float ballWidth = ballHeight * ballSource.w / ballSource.h;
            const SDL_FPoint ballScreen = camera.worldToScreen(ballX, ballY);
            SDL_FRect ballDestination = {
                ballScreen.x - ballWidth * 0.5f,
                ballScreen.y - ballHeight * 0.5f,
                ballWidth,
                ballHeight
            };
            SDL_RenderTexture(renderer, ballTexture, &ballSource, &ballDestination);

            //show coins collected in the current run
            const std::string coinText = "Coins: " + std::to_string(coins.getRunCoins());
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDebugText(renderer, 20.0f, 20.0f, coinText.c_str());

            //display the completed frame in the window.
            SDL_RenderPresent(renderer);

            //pause briefly between frames
            SDL_Delay(16);
        }
    }

    //clean up physics and textures
    SDL_DestroyTexture(ballTexture);
    SDL_DestroyTexture(launcherTexture);
    SDL_DestroyTexture(floorTexture);
    b2DestroyWorld(physicsWorld);
    SDL_DestroyTexture(coinTexture);

    //shut down
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
