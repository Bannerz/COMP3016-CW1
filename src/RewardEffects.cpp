#include "RewardEffects.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

RewardEffects::RewardEffects(SDL_Renderer* renderer, const std::string& assetRoot)
    : renderer(renderer)
{
    font.reset(TTF_OpenFont((assetRoot + "fonts/Lilita_One/LilitaOne-Regular.ttf").c_str(), 60));
    if (!font) throw std::runtime_error("Reward font loading failed: " + std::string(SDL_GetError()));
    bursts.reserve(maxBursts);
}

SDL_Texture* RewardEffects::label(int value) const
{
    auto found = labels.find(value);
    if (found == labels.end()) {
        //reuse textures for repeated rewards instead of rendering text on every pickup
        if (labels.size() >= maxBursts) labels.clear();
        const std::string text = "+" + std::to_string(value);
        using SurfacePtr = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;
        SurfacePtr surface(TTF_RenderText_Blended(font.get(), text.c_str(), text.size(),
                                                 {255, 255, 255, 255}), SDL_DestroySurface);
        if (!surface) throw std::runtime_error("Reward text failed: " + std::string(SDL_GetError()));
        TexturePtr texture(SDL_CreateTextureFromSurface(renderer, surface.get()), SDL_DestroyTexture);
        if (!texture) throw std::runtime_error("Reward texture failed: " + std::string(SDL_GetError()));
        SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_LINEAR);
        SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);
        found = labels.emplace(value, std::move(texture)).first;
    }
    return found->second.get();
}

void RewardEffects::add(SDL_FPoint screenPosition, int value, Kind kind)
{
    if (value <= 0) return;
    if (bursts.size() >= maxBursts) bursts.erase(bursts.begin());
    //nearby simultaneous pickups fan out so their values do not cover one another
    int neighbours = 0;
    for (const auto& burst : bursts) {
        if (burst.age < 0.12f && std::abs(burst.position.x - screenPosition.x) < 40 &&
            std::abs(burst.position.y - screenPosition.y) < 40) ++neighbours;
    }
    screenPosition.x += (neighbours % 3 - 1) * (neighbours > 0 ? 20.0f : 0.0f);
    screenPosition.y -= static_cast<float>(neighbours) * 34.0f;
    screenPosition.x = std::clamp(screenPosition.x, 40.0f, 920.0f);
    screenPosition.y = std::clamp(screenPosition.y, 75.0f, 600.0f);
    bursts.push_back({screenPosition, value, kind});
}

void RewardEffects::update(float elapsed)
{
    for (auto& burst : bursts) burst.age += std::max(0.0f, elapsed);
    std::erase_if(bursts, [](const Burst& burst) { return burst.age >= lifetime; });
}

void RewardEffects::clear()
{
    bursts.clear();
}

void RewardEffects::draw() const
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (const auto& burst : bursts) {
        const float progress = burst.age / lifetime;
        const auto alpha = static_cast<Uint8>(255 * std::min(1.0f, (1.0f - progress) / 0.4f));
        const SDL_Color colour = burst.kind == Kind::Coin ? SDL_Color{255, 222, 76, alpha} :
                                                          SDL_Color{114, 255, 193, alpha};
        //a small ring of sparks expands and falls while the reward label floats up
        for (int i = 0; i < 6; ++i) {
            const float angle = static_cast<float>(i) * std::numbers::pi_v<float> / 3.0f;
            const float radius = 9.0f + 42.0f * progress;
            const float size = 4.0f * (1.0f - progress) + 1.0f;
            const SDL_FRect spark{burst.position.x + std::cos(angle) * radius - size * 0.5f,
                burst.position.y + std::sin(angle) * radius + 26.0f * progress * progress - size * 0.5f,
                size, size};
            SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, alpha);
            SDL_RenderFillRect(renderer, &spark);
        }
        auto* texture = label(burst.value);
        const float scale = 0.5f * (1.0f + 0.2f * std::max(0.0f, 1.0f - progress * 6.0f));
        const float width = texture->w * scale;
        const float height = texture->h * scale;
        const float y = burst.position.y - 24.0f - 58.0f * progress;
        SDL_FRect destination{burst.position.x - width * 0.5f, y - height * 0.5f, width, height};
        SDL_SetTextureAlphaMod(texture, alpha);
        SDL_SetTextureColorMod(texture, 43, 30, 22);
        for (const SDL_FPoint offset : {SDL_FPoint{-2, 0}, {2, 0}, {0, -2}, {0, 2}}) {
            const SDL_FRect outline{destination.x + offset.x, destination.y + offset.y, width, height};
            SDL_RenderTexture(renderer, texture, nullptr, &outline);
        }
        SDL_SetTextureColorMod(texture, colour.r, colour.g, colour.b);
        SDL_RenderTexture(renderer, texture, nullptr, &destination);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
