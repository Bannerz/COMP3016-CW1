#pragma once

#include <SDL3/SDL.h>
#include <vector>

class Camera;

//background settings use world pixels above the floor
struct BackgroundSettings {
    SDL_Color skyAtGround = {135, 206, 235, 255};
    SDL_Color skyAtAltitude = {9, 17, 42, 255};
    float darkeningStartHeight = 600.0f;
    float darkeningEndHeight = 14000.0f;
    float mountainParallax = 0.12f;
    float cloudParallax = 0.23f;
};

//draw scenery behind the game without changing physics or world positions
class Background {
public:
    //visible cloud data is separate from drawing so placement can be checked
    struct CloudSprite {
        SDL_FRect destination;
        Uint8 opacity;
        SDL_FlipMode flip;
        int plane;
    };

    //Game owns both textures and keeps them alive while the background is used
    Background(SDL_Texture* mountains, SDL_Texture* clouds, float floorY,
               BackgroundSettings settings);

    void draw(SDL_Renderer* renderer, const Camera& camera, float ballY) const;
    SDL_Color getSkyColor(float ballY) const;
    std::vector<CloudSprite> getCloudSprites(const Camera& camera, float ballY) const;

private:
    float getAltitudeBlend(float ballY) const;
    void drawMountains(SDL_Renderer* renderer, const Camera& camera) const;
    void drawClouds(SDL_Renderer* renderer, const Camera& camera, float ballY) const;

    SDL_Texture* mountains;
    SDL_Texture* clouds;
    float floorY;
    BackgroundSettings settings;
    float mountainAspect;
    float cloudAspect;
};
