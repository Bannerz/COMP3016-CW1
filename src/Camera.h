#pragma once

#include <SDL3/SDL.h>

//handle camera movement and converting between screen and world pixels
class Camera {
public:
    Camera(float width = 960.0f, float height = 640.0f);

    void follow(float ballX, float ballY);
    void reset();

    SDL_FPoint worldToScreen(float worldX, float worldY) const;
    SDL_FPoint screenToWorld(float screenX, float screenY) const;

    float getX() const;
    float getY() const;
    float getWidth() const;
    float getHeight() const;

private:
    float x = 0.0f;
    float y = 0.0f;
    float width;
    float height;
};
