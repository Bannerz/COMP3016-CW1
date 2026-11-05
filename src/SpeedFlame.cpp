#include "SpeedFlame.h"
#include "Camera.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

SpeedFlame::SpeedFlame(SDL_Texture* texture, SpeedFlameSettings settings)
    : texture(texture), settings(settings)
{
    if (!std::isfinite(settings.startSpeed) || !std::isfinite(settings.stopSpeed) ||
        !std::isfinite(settings.fullSpeed) || !std::isfinite(settings.framesPerSecond) ||
        !std::isfinite(settings.fadeSeconds) || settings.stopSpeed < 0 ||
        settings.startSpeed <= settings.stopSpeed || settings.fullSpeed <= settings.startSpeed ||
        settings.framesPerSecond <= 0 || settings.fadeSeconds <= 0 ||
        !std::isfinite(settings.minLengthRadii) || !std::isfinite(settings.maxLengthRadii) ||
        !std::isfinite(settings.headOffsetRadii) || !std::isfinite(settings.maxOpacity) ||
        settings.minLengthRadii <= 0 || settings.maxLengthRadii < settings.minLengthRadii ||
        settings.headOffsetRadii < 0 || settings.maxOpacity < 0 || settings.maxOpacity > 1) {
        throw std::invalid_argument("Invalid speed flame settings");
    }
    if (!texture || texture->w % columns != 0 || texture->h % rows != 0) {
        throw std::invalid_argument("Speed flame requires a 5 by 4 sprite sheet");
    }
    frameWidth = static_cast<float>(texture->w) / columns;
    frameHeight = static_cast<float>(texture->h) / rows;
}

void SpeedFlame::update(SDL_FPoint velocity, bool flying, float elapsed)
{
    if (!flying) {
        reset();
        return;
    }
    const float speed = std::hypot(velocity.x, velocity.y);
    //separate on/off thresholds prevent flicker when speed hovers around the limit
    if (active && speed < settings.stopSpeed) active = false;
    else if (!active && speed >= settings.startSpeed) active = true;
    if (speed > 0.01f) direction = {velocity.x / speed, velocity.y / speed};
    const float step = std::max(0.0f, elapsed);
    const float target = active ? std::clamp((speed - settings.startSpeed) /
        (settings.fullSpeed - settings.startSpeed), 0.0f, 1.0f) : 0.0f;
    //speed controls both size and opacity; smooth changes after boosts and impacts
    const float response = 1.0f - std::exp(-step / settings.fadeSeconds);
    intensity += (target - intensity) * response;
    intensity = std::clamp(intensity, 0.0f, 1.0f);
    opacity = intensity * settings.maxOpacity;
    if (target == 0.0f && intensity < 0.001f) intensity = opacity = 0.0f;
    if (opacity > 0) {
        const float period = frameCount / settings.framesPerSecond;
        animationTime = std::fmod(animationTime + step, period);
    } else animationTime = 0;
}

void SpeedFlame::reset()
{
    animationTime = 0;
    opacity = 0;
    intensity = 0;
    active = false;
    direction = {1, 0};
}

void SpeedFlame::draw(SDL_Renderer* renderer, const Camera& camera,
                      SDL_FPoint position, float ballRadius) const
{
    if (opacity <= 0) return;
    const int frame = std::min(frameCount - 1, static_cast<int>(animationTime * settings.framesPerSecond));
    const SDL_FRect source{(frame % columns) * frameWidth, (frame / columns) * frameHeight,
                          frameWidth, frameHeight};
    const float length = ballRadius * (settings.minLengthRadii +
        intensity * (settings.maxLengthRadii - settings.minLengthRadii));
    const float height = length * frameHeight / frameWidth;
    const auto centre = camera.worldToScreen(position.x, position.y);
    //the bright head is on the right of every frame; the tail extends to the left
    //move the head past the forward edge so its rim surrounds the ball like a comet
    const SDL_FPoint pivot{length * 0.93f, height * 0.5f};
    const SDL_FRect destination{centre.x + direction.x * ballRadius * settings.headOffsetRadii - pivot.x,
                               centre.y + direction.y * ballRadius * settings.headOffsetRadii - pivot.y, length, height};
    const double angle = std::atan2(direction.y, direction.x) * 180.0 / std::numbers::pi;
    Uint8 previousAlpha = 255;
    SDL_GetTextureAlphaMod(texture, &previousAlpha);
    SDL_SetTextureAlphaMod(texture, static_cast<Uint8>(opacity * previousAlpha));
    SDL_RenderTextureRotated(renderer, texture, &source, &destination, angle, &pivot, SDL_FLIP_NONE);
    SDL_SetTextureAlphaMod(texture, previousAlpha);
}
