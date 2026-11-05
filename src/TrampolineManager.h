#pragma once

#include <SDL3/SDL.h>
#include "SpawnDensity.h"
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
    std::array<TrampolineTier, 3> tiers = {{{180.0f, 3, 65.0},
                                          {120.0f, 7, 27.0},
                                          {72.0f, 18, 8.0}}};
    float bounceSpeed = 1400.0f;
    //first, second and later landings on the same platform during a run
    std::array<float, 3> bounceMultipliers = {1.0f, 0.75f, 0.5f};
    SpawnDensitySettings density{150.0f, 1200.0f, 0.05f};
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
                      float firstX, TrampolineSettings settings = {}, float pixelsPerMetre = 40.0f);
    void startRun(float x, float y, std::optional<unsigned int> seed = std::nullopt);
    void update(float x, float y);
    void animate(float elapsed);
    std::optional<Landing> land(SDL_FPoint previous, SDL_FPoint current, float radius);
    void draw(SDL_Renderer* renderer, const Camera& camera) const;
    std::vector<Trampoline> getTrampolines() const;

private:
    using ChunkId = std::pair<int, int>;
    std::array<SDL_Texture*, 3> textures;
    TrampolineSettings settings;
    float floorY;
    float firstX;
    float pixelsPerMetre;
    float runStartX = 0;
    static constexpr float chunkWidth = 960.0f;
    static constexpr float chunkHeight = 640.0f;
    std::map<ChunkId, Trampoline> chunks;
    //keep landing history after unloading so returning does not restore thrust or rewards
    std::map<ChunkId, std::size_t> landingCounts;
    std::set<ChunkId> generatedChunks;
    unsigned int runSeed = 0;
};
