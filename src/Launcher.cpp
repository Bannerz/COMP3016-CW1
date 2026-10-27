#include "Launcher.h"
#include "Ball.h"
#include "Camera.h"

#include <cmath>

Launcher::Launcher(SDL_Texture* texture, SDL_FPoint position, SDL_FPoint size,
                   SDL_FPoint anchorOffset, float launchPower, float maxDragDistance)
    : texture(texture), position(position), size(size), anchorOffset(anchorOffset),
      launchPower(launchPower), maxDragDistance(maxDragDistance)
{
}

SDL_FPoint Launcher::getAnchor() const
{
    return {position.x + anchorOffset.x, position.y + anchorOffset.y};
}

SDL_FPoint Launcher::clampPull(SDL_FPoint mousePosition) const
{
    const SDL_FPoint anchor = getAnchor();
    float pullX = mousePosition.x - anchor.x;
    float pullY = mousePosition.y - anchor.y;
    const float distance = std::sqrt(pullX * pullX + pullY * pullY);

    //limit how far the ball can be pulled from the launcher
    if (distance > maxDragDistance) {
        const float scale = maxDragDistance / distance;
        pullX *= scale;
        pullY *= scale;
    }
    return {anchor.x + pullX, anchor.y + pullY};
}

void Launcher::handleEvent(const SDL_Event& event, const Camera& camera, Ball& ball) const
{
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        const SDL_FPoint mouse = camera.screenToWorld(event.button.x, event.button.y);
        if (ball.beginDrag(mouse)) {
            SDL_CaptureMouse(true);
        }
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION && ball.getState() == Ball::State::Dragging) {
        ball.dragTo(clampPull(camera.screenToWorld(event.motion.x, event.motion.y)));
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT &&
        ball.getState() == Ball::State::Dragging) {
        //include the final mouse position before launching
        ball.dragTo(clampPull(camera.screenToWorld(event.button.x, event.button.y)));
        SDL_CaptureMouse(false);
        const SDL_FPoint anchor = getAnchor();
        const SDL_FPoint ballPosition = ball.getPosition();
        ball.launch({(anchor.x - ballPosition.x) * launchPower,
                     (anchor.y - ballPosition.y) * launchPower});
    }

    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST && ball.getState() == Ball::State::Dragging) {
        //cancel the pull if the player switches away from the window
        ball.cancelDrag();
        SDL_CaptureMouse(false);
    }
}

void Launcher::draw(SDL_Renderer* renderer, const Camera& camera) const
{
    const SDL_FPoint screen = camera.worldToScreen(position.x, position.y);
    SDL_FRect destination = {screen.x, screen.y, size.x, size.y};
    SDL_RenderTexture(renderer, texture, nullptr, &destination);
}
