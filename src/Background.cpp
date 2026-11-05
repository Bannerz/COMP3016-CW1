#include "Background.h"
#include "Camera.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace {
//use separate random samples for position, size and opacity in each region
std::uint32_t cloudHash(int column, int row, std::uint32_t salt)
{
    std::uint32_t value = static_cast<std::uint32_t>(column) * 73856093u ^
                          static_cast<std::uint32_t>(row) * 19349663u ^ salt * 83492791u;
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    return value;
}

float cloudSample(int column, int row, std::uint32_t salt)
{
    return static_cast<float>(cloudHash(column, row, salt) & 0x00ffffffu) / 16777216.0f;
}

struct CloudPlane {
    float regionWidth;
    float regionHeight;
    float minWidth;
    float maxWidth;
    float parallaxScale;
    float opacity;
    SDL_FPoint originOffset;
};

//small hazy clouds sit farther away, while larger clouds move nearer the player
constexpr std::array<CloudPlane, 3> cloudPlanes = {{
    {700.0f, 480.0f, 120.0f, 230.0f, 0.45f, 100.0f, {0.0f, -40.0f}},
    {1040.0f, 640.0f, 220.0f, 390.0f, 0.95f, 145.0f, {-680.0f, 140.0f}},
    {1440.0f, 900.0f, 340.0f, 620.0f, 1.55f, 185.0f, {200.0f, 260.0f}}
}};
} //namespace

Background::Background(SDL_Texture* mountains, SDL_Texture* clouds, float floorY,
                       BackgroundSettings settings)
    : mountains(mountains), clouds(clouds), floorY(floorY), settings(settings)
{
    if (settings.darkeningStartHeight < 0.0f ||
        settings.darkeningEndHeight <= settings.darkeningStartHeight ||
        settings.mountainParallax < 0.0f || settings.mountainParallax > 1.0f ||
        settings.cloudParallax < 0.0f || settings.cloudParallax > 1.0f ||
        !std::isfinite(settings.starParallax) || settings.starParallax < 0.0f || settings.starParallax > 1.0f) {
        throw std::invalid_argument("Invalid background altitude or parallax settings");
    }

    float mountainWidth = 0.0f;
    float mountainHeight = 0.0f;
    float cloudWidth = 0.0f;
    float cloudHeight = 0.0f;
    if (!mountains || !clouds ||
        !SDL_GetTextureSize(mountains, &mountainWidth, &mountainHeight) ||
        !SDL_GetTextureSize(clouds, &cloudWidth, &cloudHeight) ||
        mountainWidth <= 0.0f || cloudWidth <= 0.0f) {
        throw std::runtime_error("Could not read background texture sizes");
    }
    mountainAspect = mountainHeight / mountainWidth;
    cloudAspect = cloudHeight / cloudWidth;
}

float Background::getAltitudeBlend(float ballY) const
{
    const float height = std::max(0.0f, floorY - ballY);
    const float blend = std::clamp(
        (height - settings.darkeningStartHeight) /
        (settings.darkeningEndHeight - settings.darkeningStartHeight), 0.0f, 1.0f
    );
    //ease the colour change at both ends rather than switching suddenly
    return blend * blend * (3.0f - 2.0f * blend);
}

SDL_Color Background::getSkyColor(float ballY) const
{
    const float blend = getAltitudeBlend(ballY);
    const auto mix = [blend](Uint8 low, Uint8 high) {
        return static_cast<Uint8>(std::lround(low + (static_cast<float>(high) - low) * blend));
    };
    return {mix(settings.skyAtGround.r, settings.skyAtAltitude.r),
            mix(settings.skyAtGround.g, settings.skyAtAltitude.g),
            mix(settings.skyAtGround.b, settings.skyAtAltitude.b), 255};
}

void Background::drawStars(SDL_Renderer* renderer, const Camera& camera, float ballY) const
{
    //use the same altitude blend as the sky so stars fade back out on descent
    const float visibility = std::clamp((getAltitudeBlend(ballY) - 0.12f) / 0.88f, 0.0f, 1.0f);
    if (visibility <= 0.0f) return;
    const float fade = visibility * visibility * (3.0f - 2.0f * visibility);
    constexpr float regionWidth = 240.0f;
    constexpr float regionHeight = 200.0f;
    constexpr float margin = 6.0f;
    const float cameraX = camera.getX() * settings.starParallax;
    const float cameraY = camera.getY() * settings.starParallax;
    const int firstColumn = static_cast<int>(std::floor((cameraX - margin) / regionWidth));
    const int lastColumn = static_cast<int>(std::floor((cameraX + camera.getWidth() + margin) / regionWidth));
    const int firstRow = static_cast<int>(std::floor((cameraY - margin) / regionHeight));
    const int lastRow = static_cast<int>(std::floor((cameraY + camera.getHeight() + margin) / regionHeight));
    SDL_BlendMode previousBlend;
    SDL_GetRenderDrawBlendMode(renderer, &previousBlend);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            //hash world regions instead of generating new random positions each frame
            const Uint32 regionSalt = settings.starSeed * 131u + 0x57a2u;
            const int count = 3 + static_cast<int>(cloudHash(column, row, regionSalt) % 4u);
            for (int slot = 0; slot < count; ++slot) {
                const Uint32 salt = regionSalt + static_cast<Uint32>(slot + 1) * 17u;
                const float x = (column + cloudSample(column, row, salt)) * regionWidth - cameraX;
                const float y = (row + cloudSample(column, row, salt + 1)) * regionHeight - cameraY;
                if (x < -margin || y < -margin || x > camera.getWidth() + margin ||
                    y > camera.getHeight() + margin) continue;
                const float brightness = cloudSample(column, row, salt + 2);
                const bool bright = brightness > 0.86f;
                const float size = bright ? 2.2f : 1.0f + brightness * 0.7f;
                const auto alpha = static_cast<Uint8>(settings.starColour.a * fade * (0.35f + brightness * 0.65f));
                const auto& colour = settings.starColour;
                if (bright) {
                    //a faint halo and short cross give only the brightest stars extra detail
                    SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, alpha / 9);
                    const SDL_FRect halo{x - 3.0f, y - 3.0f, 6.0f, 6.0f};
                    SDL_RenderFillRect(renderer, &halo);
                    SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, alpha / 3);
                    SDL_RenderLine(renderer, x - 3.5f, y, x + 3.5f, y);
                    SDL_RenderLine(renderer, x, y - 3.5f, x, y + 3.5f);
                }
                SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, alpha);
                const SDL_FRect star{x - size * 0.5f, y - size * 0.5f, size, size};
                SDL_RenderFillRect(renderer, &star);
            }
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, previousBlend);
}

void Background::drawMountains(SDL_Renderer* renderer, const Camera& camera) const
{
    const float width = 870.0f;
    const float height = width * mountainAspect;
    //overlap the transparent edges of the image to avoid gaps in the range
    const float spacing = width * 0.90f;
    float offset = std::fmod(camera.getX() * settings.mountainParallax, spacing);
    if (offset < 0.0f) {
        offset += spacing;
    }
    //keep mountain bases attached to the ground when looking up
    //only horizontal movement uses parallax
    const float y = floorY + 36.0f - height - camera.getY();
    if (y > camera.getHeight() || y + height < 0.0f) {
        return;
    }

    //soften distant mountains against the sky
    SDL_SetTextureAlphaMod(mountains, 190);
    for (float x = -offset - spacing; x < camera.getWidth(); x += spacing) {
        SDL_FRect destination = {x, y, width, height};
        SDL_RenderTexture(renderer, mountains, nullptr, &destination);
    }
    SDL_SetTextureAlphaMod(mountains, 255);
}

std::vector<Background::CloudSprite> Background::getCloudSprites(const Camera& camera, float ballY) const
{
    std::vector<CloudSprite> sprites;
    const float altitudeBlend = getAltitudeBlend(ballY);
    if (altitudeBlend >= 1.0f) {
        return sprites;
    }
    sprites.reserve(24);

    for (std::size_t planeIndex = 0; planeIndex < cloudPlanes.size(); ++planeIndex) {
        const CloudPlane& plane = cloudPlanes[planeIndex];
        //offset the planes independently so they do not share the same starting pattern
        const float cameraX = camera.getX() * settings.cloudParallax * plane.parallaxScale -
                              plane.originOffset.x;
        const float cameraY = camera.getY() * 0.28f * plane.parallaxScale - plane.originOffset.y;
        const float maxHalfWidth = plane.maxWidth * 0.5f;
        const float maxHalfHeight = plane.maxWidth * cloudAspect * 0.5f;
        const int firstColumn = static_cast<int>(std::floor((cameraX - maxHalfWidth) / plane.regionWidth));
        const int lastColumn = static_cast<int>(std::floor((cameraX + camera.getWidth() + maxHalfWidth) / plane.regionWidth));
        const int firstRow = static_cast<int>(std::floor((cameraY - maxHalfHeight) / plane.regionHeight));
        const int lastRow = static_cast<int>(std::floor((cameraY + camera.getHeight() + maxHalfHeight) / plane.regionHeight));

        //regions only bound generation; clouds can appear anywhere inside them
        //varying the count and leaving empty regions avoids visible rows and columns
        for (int row = firstRow; row <= lastRow; ++row) {
            for (int column = firstColumn; column <= lastColumn; ++column) {
                const auto planeSalt = static_cast<std::uint32_t>(planeIndex + 1) * 101u;
                const int candidates = 1 + static_cast<int>(cloudHash(column, row, planeSalt) % 3u);
                for (int slot = 0; slot < candidates; ++slot) {
                    const auto salt = planeSalt + static_cast<std::uint32_t>(slot + 1) * 11u;
                    if (cloudSample(column, row, salt) > 0.65f) {
                        continue;
                    }
                    const float width = plane.minWidth + (plane.maxWidth - plane.minWidth) *
                                        cloudSample(column, row, salt + 1);
                    const float height = width * cloudAspect;
                    const float x = (column + cloudSample(column, row, salt + 2)) * plane.regionWidth -
                                    cameraX - width * 0.5f;
                    const float y = (row + cloudSample(column, row, salt + 3)) * plane.regionHeight -
                                    cameraY - height * 0.5f;
                    if (x + width < 0.0f || x > camera.getWidth() ||
                        y + height < 0.0f || y > camera.getHeight()) {
                        continue;
                    }
                    const float opacity = plane.opacity *
                        (0.8f + 0.2f * cloudSample(column, row, salt + 4)) * (1.0f - altitudeBlend);
                    const SDL_FlipMode flip = cloudSample(column, row, salt + 5) < 0.5f ?
                                             SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
                    sprites.push_back({{x, y, width, height}, static_cast<Uint8>(opacity),
                                       flip, static_cast<int>(planeIndex)});
                }
            }
        }
    }
    return sprites;
}

void Background::drawClouds(SDL_Renderer* renderer, const Camera& camera, float ballY) const
{
    //draw the far plane first so nearer clouds overlap it naturally
    for (const CloudSprite& cloud : getCloudSprites(camera, ballY)) {
        SDL_SetTextureAlphaMod(clouds, cloud.opacity);
        SDL_RenderTextureRotated(renderer, clouds, nullptr, &cloud.destination,
                                 0.0, nullptr, cloud.flip);
    }
    SDL_SetTextureAlphaMod(clouds, 255);
}

void Background::draw(SDL_Renderer* renderer, const Camera& camera, float ballY) const
{
    const SDL_Color sky = getSkyColor(ballY);
    SDL_SetRenderDrawColor(renderer, sky.r, sky.g, sky.b, sky.a);
    SDL_RenderClear(renderer);

    drawStars(renderer, camera, ballY);
    drawMountains(renderer, camera);
    drawClouds(renderer, camera, ballY);
}
