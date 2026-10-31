#include "CoinManager.h"
#include "Camera.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <numbers>
#include <stdexcept>

CoinManager::CoinManager(std::array<SDL_Texture*, 3> textures, float floorY, float firstCoinX,
                         std::array<double, 3> valueWeights)
    : textures(textures), valueWeights(valueWeights), floorY(floorY), firstCoinX(firstCoinX)
{
    setValueWeights(valueWeights);
}

void CoinManager::setValueWeights(std::array<double, 3> weights)
{
    const double total = std::accumulate(weights.begin(), weights.end(), 0.0);
    if (!std::isfinite(total) || total <= 0.0 ||
        std::any_of(weights.begin(), weights.end(), [](double weight) {
            return !std::isfinite(weight) || weight < 0.0;
        })) {
        throw std::invalid_argument("Coin value weights must be nonnegative with a positive total");
    }
    valueWeights = weights;
}

void CoinManager::startRun(float startX, float startY)
{
    //start each attempt with a fresh layout and pickup history
    const unsigned int previousSeed = runSeed;
    do {
        runSeed = seed();
    } while (runSeed == previousSeed);

    chunks.clear();
    collectedCoins.clear();
    runCoins = 0;
    pickups.clear();
    update(startX, startY);
}

void CoinManager::update(float worldX, float worldY)
{
    const int currentChunkX =
        static_cast<int>(std::floor(worldX / chunkWidth));

    //vertical section numbers increase upwards from the floor
    const float heightAboveFloor = std::max(0.0f, floorY - worldY);
    const int currentChunkY =
        static_cast<int>(std::floor(heightAboveFloor / chunkHeight));

    const int firstX = currentChunkX - 1;
    const int lastX = currentChunkX + 1;
    const int firstY = std::max(0, currentChunkY - 1);
    const int lastY = currentChunkY + 1;

    //create any missing sections
    for (int chunkX = firstX; chunkX <= lastX; ++chunkX) {
        for (int chunkY = firstY; chunkY <= lastY; ++chunkY) {
            const ChunkId chunkId = {chunkX, chunkY};
            if (chunks.contains(chunkId)) {
                continue;
            }

            //reloading a section gives it the same layout during this run
            std::seed_seq chunkSeed{
                runSeed,
                static_cast<unsigned int>(chunkX),
                static_cast<unsigned int>(chunkY)
            };
            std::mt19937 chunkRandom(chunkSeed);
            //use a separate generator so adding coin values does not move the coins
            std::seed_seq valueSeed{
                runSeed, static_cast<unsigned int>(chunkX),
                static_cast<unsigned int>(chunkY), 0xc01f01u
            };
            std::mt19937 valueRandom(valueSeed);
            std::discrete_distribution<int> coinType(valueWeights.begin(), valueWeights.end());
            std::uniform_real_distribution<float> randomX(
                48.0f, chunkWidth - 48.0f
            );
            std::uniform_real_distribution<float> randomHeight(
                48.0f, chunkHeight - 48.0f
            );

            std::vector<Coin> chunkCoins;
            for (int slot = 0; slot < coinsPerChunk; ++slot) {
                const int value = coinType(valueRandom) + 1;
                const float x =
                    chunkX * chunkWidth + randomX(chunkRandom);
                const float y = floorY - (
                    chunkY * chunkHeight + randomHeight(chunkRandom)
                );

                //leave the launcher area clear
                if (x < firstCoinX) {
                    continue;
                }

                const CoinId coinId = {chunkX, chunkY, slot};
                chunkCoins.push_back({
                    x, y, collectedCoins.contains(coinId), slot, value
                });
            }
            chunks.emplace(chunkId, std::move(chunkCoins));
        }
    }

    //remove sections that are no longer nearby
    for (auto it = chunks.begin(); it != chunks.end();) {
        const int chunkX = it->first.first;
        const int chunkY = it->first.second;
        if (chunkX < firstX || chunkX > lastX ||
            chunkY < firstY || chunkY > lastY) {
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }
}

int CoinManager::collect(float ballX, float ballY, float ballRadius, float extraReach)
{
    //return the total value collected during this check
    int awardedValue = 0;
    pickups.clear();
    for (auto& [chunkId, chunkCoins] : chunks) {
        for (Coin& coin : chunkCoins) {
            if (coin.collected) {
                continue;
            }

            const float dx = ballX - coin.x;
            const float dy = ballY - coin.y;
            const float pickupDistance = ballRadius + coinRadius + std::max(0.0f, extraReach);

            if (dx * dx + dy * dy <= pickupDistance * pickupDistance) {
                coin.collected = true;
                runCoins += coin.value;
                awardedValue += coin.value;
                pickups.push_back({{coin.x, coin.y}, coin.value});

                //keep this coin collected if its section is loaded again
                collectedCoins.emplace(chunkId.first, chunkId.second, coin.slot);
            }
        }
    }
    return awardedValue;
}

const std::vector<CoinManager::Pickup>& CoinManager::getPickups() const
{
    return pickups;
}

void CoinManager::drawPickupRange(SDL_Renderer* renderer, const Camera& camera,
                                  SDL_FPoint ballPosition, float ballRadius, float extraReach) const
{
    if (extraReach <= 0) return;
    const SDL_FPoint centre = camera.worldToScreen(ballPosition.x, ballPosition.y);
    //the ring marks the catchers edge; a coins edge touching it counts as a pickup
    const float radius = ballRadius + extraReach;
    constexpr std::size_t segments = 96;
    std::array<SDL_FPoint, segments + 1> points{};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int pass = 0; pass < 5; ++pass) {
        const float edge = radius + static_cast<float>(pass - 2);
        for (std::size_t i = 0; i < segments; ++i) {
            const float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / segments;
            points[i] = {centre.x + std::cos(angle) * edge, centre.y + std::sin(angle) * edge};
        }
        points[segments] = points[0];
        //a dark rim keeps the gold outline visible against pale skies and bright assets
        if (pass > 0 && pass < 4) SDL_SetRenderDrawColor(renderer, 255, 222, 76, 210);
        else SDL_SetRenderDrawColor(renderer, 57, 40, 29, 145);
        SDL_RenderLines(renderer, points.data(), static_cast<int>(points.size()));
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void CoinManager::draw(SDL_Renderer* renderer, const Camera& camera) const
{
    //draw uncollected coins in the nearby sections
    for (const auto& chunk : chunks) {
        for (const Coin& coin : chunk.second) {
            if (coin.collected) {
                continue;
            }

            const SDL_FPoint screen = camera.worldToScreen(coin.x, coin.y);
            if (screen.x + coinRadius < 0.0f ||
                screen.x - coinRadius > camera.getWidth() ||
                screen.y + coinRadius < 0.0f ||
                screen.y - coinRadius > camera.getHeight()) {
                continue;
            }

            SDL_FRect destination = {
                screen.x - coinRadius,
                screen.y - coinRadius,
                coinSize,
                coinSize
            };
            SDL_Texture* texture = textures.at(static_cast<std::size_t>(coin.value - 1));
            SDL_RenderTexture(renderer, texture, nullptr, &destination);
        }
    }
}

int CoinManager::getRunCoins() const
{
    return runCoins;
}

void CoinManager::addCoins(int value)
{
    //trampoline rewards use the same run balance as collected coins
    runCoins += std::max(0, value);
}
