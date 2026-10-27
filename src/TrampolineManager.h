#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <utility>
#include <vector>

class Camera;

struct TrampolineTier {
    float width;
    int reward;
    double weight;
};

struct TrampolineSettings {
    //green, blue and purple get smaller, more valuable and rarer
    std::array<TrampolineTier, 3> tiers = {{{180.0f, 5, 65.0},
                                          {120.0f, 10, 27.0},
                                          {72.0f, 25, 8.0}}};
    float bounceSpeed = 1800.0f;
};

//stream floating platforms in both directions and keep their reward history
class TrampolineManager {
public:
    struct Trampoline {
        float x;
        float surfaceY;
        std::size_t tier;
        float bounceTime = 1.0f;

        float getVisualOffset() const;
    };
    struct Landing {
        SDL_FPoint position;
        float upwardSpeed;
        int reward;
    };

    TrampolineManager(std::array<SDL_Texture*, 3> textures, float floorY,
                      float firstX, TrampolineSettings settings = {});
    void startRun(float x, float y, std::optional<unsigned int> seed = std::nullopt);
    void update(float x, float y);
    void animate(float elapsed);
    std::optional<Landing> land(SDL_FPoint previous, SDL_FPoint current, float radius);
    void draw(SDL_Renderer* renderer, const Camera& camera) const;
    int getRunScore() const;
    std::vector<Trampoline> getTrampolines() const;

private:
    using ChunkId = std::pair<int, int>;
    std::array<SDL_Texture*, 3> textures;
    TrampolineSettings settings;
    float floorY;
    float firstX;
    static constexpr float chunkWidth = 960.0f;
    static constexpr float chunkHeight = 640.0f;
    std::map<ChunkId, Trampoline> chunks;
    std::set<ChunkId> rewarded;
    unsigned int runSeed = 0;
    int runScore = 0;
};
