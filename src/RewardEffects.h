#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

//short lived reward feedback stays readable while the camera follows a fast launch
class RewardEffects {
public:
    enum class Kind { Coin, Trampoline };
    RewardEffects(SDL_Renderer* renderer, const std::string& assetRoot);
    void add(SDL_FPoint screenPosition, int value, Kind kind);
    void update(float elapsed);
    void clear();
    void draw() const;

private:
    using TexturePtr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
    using FontPtr = std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)>;
    struct Burst {
        SDL_FPoint position;
        int value;
        Kind kind;
        float age = 0;
    };
    SDL_Texture* label(int value) const;
    SDL_Renderer* renderer;
    FontPtr font{nullptr, TTF_CloseFont};
    mutable std::map<int, TexturePtr> labels;
    std::vector<Burst> bursts;
    static constexpr float lifetime = 0.9f;
    static constexpr std::size_t maxBursts = 64;
};
