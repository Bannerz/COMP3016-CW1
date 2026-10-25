#include "Camera.h"

#include <algorithm>

Camera::Camera(float width, float height)
    : width(width), height(height)
{
}

void Camera::follow(float ballX, float ballY)
{
    //follow horizontally after 35 percent and upwards after 40 percent
    x = std::max(0.0f, ballX - width * 0.35f);
    y = std::min(0.0f, ballY - height * 0.40f);
}

void Camera::reset()
{
    x = 0.0f;
    y = 0.0f;
}

SDL_FPoint Camera::worldToScreen(float worldX, float worldY) const
{
    //subtract the camera when drawing world objects
    return {worldX - x, worldY - y};
}

SDL_FPoint Camera::screenToWorld(float screenX, float screenY) const
{
    //add the camera when reading mouse positions
    return {screenX + x, screenY + y};
}

float Camera::getX() const { return x; }
float Camera::getY() const { return y; }
float Camera::getWidth() const { return width; }
float Camera::getHeight() const { return height; }
