#include "Launcher.h"
#include "Ball.h"
#include "Camera.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
struct BandStyle {
    SDL_FColor colour;
    float width;
};

//Starter rubber, then a distinct colour and heavier band for each purchased level.
constexpr std::array<BandStyle, 6> bandStyles = {{
    {{0.88f, 0.66f, 0.30f, 1.0f}, 6.0f}, //tan
    {{0.24f, 0.78f, 0.42f, 1.0f}, 7.0f}, //green
    {{0.12f, 0.80f, 0.85f, 1.0f}, 8.0f}, //cyan
    {{0.24f, 0.43f, 0.95f, 1.0f}, 9.0f}, //blue
    {{0.68f, 0.30f, 0.92f, 1.0f}, 10.0f}, //violet
    {{1.00f, 0.76f, 0.12f, 1.0f}, 11.0f} //gold
}};

struct ForkAttachments {
    SDL_FPoint left;
    SDL_FPoint right;
};

//normalised centres of the metal fork collars in each equipped launcher image
//launcher upgrades change these points independently of band colour and thickness
constexpr std::array<ForkAttachments, 6> forkAttachments = {{
    {{0.19f, 0.145f}, {0.81f, 0.145f}}, //level 00
    {{0.18f, 0.145f}, {0.82f, 0.145f}}, //level 01
    {{0.233f, 0.105f}, {0.754f, 0.105f}}, //level 02
    {{0.243f, 0.102f}, {0.750f, 0.102f}}, //level 03
    {{0.284f, 0.074f}, {0.709f, 0.074f}}, //level 04
    {{0.177f, 0.143f}, {0.827f, 0.133f}} //level 05-fixed
}};

//A curved ribbon gives the rubber a little slack at rest, becoming taut under load.
void drawBand(SDL_Renderer* renderer, SDL_FPoint start, SDL_FPoint end,
              float width, float slack, SDL_FColor colour)
{
    constexpr int segments = 16;
    std::array<SDL_Vertex, (segments + 1) * 2> vertices{};
    std::array<int, segments * 6> indices{};
    for (int i = 0; i <= segments; ++i) {
        const float t = static_cast<float>(i) / segments;
        const SDL_FPoint centre = {
            start.x + (end.x - start.x) * t,
            start.y + (end.y - start.y) * t + 4.0f * slack * t * (1.0f - t)
        };
        const float dx = end.x - start.x;
        const float dy = end.y - start.y + 4.0f * slack * (1.0f - 2.0f * t);
        const float length = std::sqrt(dx * dx + dy * dy);
        const float scale = length > 0.001f ? width * 0.5f / length : 0.0f;
        vertices[i * 2] = {{centre.x - dy * scale, centre.y + dx * scale}, colour, {0, 0}};
        vertices[i * 2 + 1] = {{centre.x + dy * scale, centre.y - dx * scale}, colour, {0, 0}};
        if (i < segments) {
            const int base = i * 2;
            const int offset = i * 6;
            indices[offset] = base;
            indices[offset + 1] = base + 1;
            indices[offset + 2] = base + 2;
            indices[offset + 3] = base + 1;
            indices[offset + 4] = base + 3;
            indices[offset + 5] = base + 2;
        }
    }
    SDL_RenderGeometry(renderer, nullptr, vertices.data(), static_cast<int>(vertices.size()),
                       indices.data(), static_cast<int>(indices.size()));
}
}

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

void Launcher::configure(SDL_Texture* sprite, float power, float pullDistance, int bandUpgrade, int launcherUpgrade)
{
    //apply purchased equipment before a new attempt starts
    texture = sprite;
    launchPower = power;
    maxDragDistance = pullDistance;
    bandsLevel = std::clamp(bandUpgrade, 0, static_cast<int>(bandStyles.size()) - 1);
    launcherLevel = std::clamp(launcherUpgrade, 0, static_cast<int>(forkAttachments.size()) - 1);
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

void Launcher::draw(SDL_Renderer* renderer, const Camera& camera, const Ball& ball) const
{
    const SDL_FPoint screen = camera.worldToScreen(position.x, position.y);
    SDL_FRect destination = {screen.x, screen.y, size.x, size.y};
    SDL_RenderTexture(renderer, texture, nullptr, &destination);

    //Released bands stay on the fork instead of following the ball into flight.
    const SDL_FPoint anchor = getAnchor();
    const SDL_FPoint pouch = ball.getState() == Ball::State::Flying ? anchor : ball.getPosition();
    const float dx = pouch.x - anchor.x;
    const float dy = pouch.y - anchor.y;
    const float tension = maxDragDistance > 0.0f
        ? std::clamp(std::sqrt(dx * dx + dy * dy) / maxDragDistance, 0.0f, 1.0f) : 0.0f;
    const BandStyle& style = bandStyles[static_cast<std::size_t>(bandsLevel)];
    const float width = style.width * (1.0f - tension / 3.0f);
    const float slack = 6.0f * (1.0f - tension) * (1.0f - tension);
    //Brighten under tension while preserving the upgrade's identifying colour.
    const auto lighten = [](SDL_FColor colour, float amount) {
        return SDL_FColor{colour.r + (1.0f - colour.r) * amount,
                          colour.g + (1.0f - colour.g) * amount,
                          colour.b + (1.0f - colour.b) * amount, 1.0f};
    };
    const SDL_FColor rubber = lighten(style.colour, tension * 0.20f);
    const SDL_FColor outline = {style.colour.r * 0.28f, style.colour.g * 0.28f,
                               style.colour.b * 0.28f, 1.0f};
    const SDL_FColor highlight = lighten(rubber, 0.45f);
    const SDL_FPoint end = camera.worldToScreen(pouch.x, pouch.y);
    //use the equipped launchers collar positions, rather than one set for all artwork
    const auto& forks = forkAttachments[static_cast<std::size_t>(launcherLevel)];
    for (const SDL_FPoint fork : {forks.left, forks.right}) {
        const SDL_FPoint start = camera.worldToScreen(position.x + size.x * fork.x,
                                                     position.y + size.y * fork.y);
        drawBand(renderer, start, end, width + 2.0f, slack, outline);
        drawBand(renderer, start, end, width, slack, rubber);
        drawBand(renderer, start, end, width * 0.25f, slack, highlight);
    }
}
