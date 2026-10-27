#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <map>
#include <random>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

class Camera;

//handle coin generation, pickups and drawing for each attempt
class CoinManager {
public:
    //textures are ordered by value: 1, 2 and 3, and are owned by Game
    CoinManager(std::array<SDL_Texture*, 3> textures, float floorY, float firstCoinX,
                std::array<double, 3> valueWeights = {60.0, 28.0, 12.0});

    void startRun(float startX, float startY);
    void update(float ballX, float ballY);
    //collection and the run total use coin values, not the number of objects
    int collect(float ballX, float ballY, float ballRadius);
    void draw(SDL_Renderer* renderer, const Camera& camera) const;
    int getRunCoins() const;

private:
    //store each coins position, collected state and number within its section
    struct Coin {
        float x;
        float y;
        bool collected = false;
        int slot = 0;
        int value = 1;
    };

    using ChunkId = std::pair<int, int>;
    using CoinId = std::tuple<int, int, int>;

    std::array<SDL_Texture*, 3> textures;
    std::array<double, 3> valueWeights;
    float floorY;
    float firstCoinX;

    static constexpr float coinSize = 32.0f;
    static constexpr float coinRadius = coinSize * 0.5f;
    static constexpr float chunkWidth = 960.0f;
    static constexpr float chunkHeight = 640.0f;
    static constexpr int coinsPerChunk = 8;

    std::map<ChunkId, std::vector<Coin>> chunks;
    std::set<CoinId> collectedCoins;
    std::random_device seed;
    unsigned int runSeed = 0;
    int runCoins = 0;
};
