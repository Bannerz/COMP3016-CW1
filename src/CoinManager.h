#pragma once

#include <SDL3/SDL.h>
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
    //main owns the texture and keeps it alive while coins are being drawn
    CoinManager(SDL_Texture* texture, float floorY, float firstCoinX);

    void startRun(float startX, float startY);
    void update(float ballX, float ballY);
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
    };

    using ChunkId = std::pair<int, int>;
    using CoinId = std::tuple<int, int, int>;

    SDL_Texture* texture;
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
