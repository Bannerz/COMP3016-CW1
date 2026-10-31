#pragma once

#include <SDL3/SDL.h>
#include <algorithm>

//keep run measurements in metres and seconds, independent of the camera
class RunStats {
public:
    explicit RunStats(float pixelsPerMetre) : pixelsPerMetre(pixelsPerMetre) {}

    void reset(float launcherX)
    {
        startX = launcherX;
        distance = 0.0f;
        height = 0.0f;
        bestHeight = 0.0f;
        duration = 0.0;
    }

    void update(SDL_FPoint position, float floorY, float ballRadius, double elapsed = 0.0)
    {
        //backtracking does not reduce distance, and resting on the floor means zero height
        distance = std::max(distance, (position.x - startX) / pixelsPerMetre);
        height = std::max(0.0f, (floorY - position.y - ballRadius) / pixelsPerMetre);
        bestHeight = std::max(bestHeight, height);
        duration += std::max(0.0, elapsed);
    }

    float getDistance() const { return distance; }
    float getHeight() const { return height; }
    float getBestHeight() const { return bestHeight; }
    double getDuration() const { return duration; }

private:
    float pixelsPerMetre;
    float startX = 0.0f;
    float distance = 0.0f;
    float height = 0.0f;
    float bestHeight = 0.0f;
    double duration = 0.0;
};
