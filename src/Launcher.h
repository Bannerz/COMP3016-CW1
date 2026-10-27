#pragma once

#include <SDL3/SDL.h>

class Ball;
class Camera;

//handle the launcher sprite, anchor point and pull back controls
class Launcher {
public:
    //the game owns the texture
    Launcher(SDL_Texture* texture, SDL_FPoint position, SDL_FPoint size,
             SDL_FPoint anchorOffset, float launchPower, float maxDragDistance);

    void handleEvent(const SDL_Event& event, const Camera& camera, Ball& ball) const;
    void draw(SDL_Renderer* renderer, const Camera& camera) const;
    SDL_FPoint getAnchor() const;

private:
    SDL_FPoint clampPull(SDL_FPoint mousePosition) const;

    SDL_Texture* texture;
    SDL_FPoint position;
    SDL_FPoint size;
    SDL_FPoint anchorOffset;
    float launchPower;
    float maxDragDistance;
};
