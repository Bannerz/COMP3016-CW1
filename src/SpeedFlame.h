#pragma once

#include <SDL3/SDL.h>

class Camera;

//speed thresholds are metres per second, independent of camera motion
struct SpeedFlameSettings {
    float startSpeed = 35.0f;
    float stopSpeed = 30.0f;
    float fullSpeed = 100.0f;
    float framesPerSecond = 24.0f;
    float fadeSeconds = 0.15f;
    //length and leading-head offset are multiples of the balls collision radius
    float minLengthRadii = 4.0f;
    float maxLengthRadii = 7.0f;
    float headOffsetRadii = 1.15f;
    float maxOpacity = 1.0f;
};

//animate a comet envelope around the ball without changing its physics
class SpeedFlame {
public:
    SpeedFlame(SDL_Texture* texture, SpeedFlameSettings settings = {});
    //velocity uses screen directions: positive x right, positive y down
    void update(SDL_FPoint velocity, bool flying, float elapsed);
    void reset();
    void draw(SDL_Renderer* renderer, const Camera& camera, SDL_FPoint position, float ballRadius) const;

private:
    SDL_Texture* texture;
    SpeedFlameSettings settings;
    SDL_FPoint direction{1, 0};
    float frameWidth;
    float frameHeight;
    float animationTime = 0;
    float opacity = 0;
    float intensity = 0;
    bool active = false;
    static constexpr int columns = 5;
    static constexpr int rows = 4;
    static constexpr int frameCount = columns * rows;
};
