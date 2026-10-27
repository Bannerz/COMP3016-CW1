#include "TrampolineManager.h"
#include "Camera.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace {
constexpr float bounceDuration = 0.9f;
}

float TrampolineManager::Trampoline::getVisualOffset() const
{
    //dip first, then wobble three times with smaller movements until resting
    const float progress = std::clamp(bounceTime / bounceDuration, 0.0f, 1.0f);
    const float remaining = 1.0f - progress;
    return 8.0f * remaining * remaining *
           std::sin(progress * 6.0f * std::numbers::pi_v<float>);
}

TrampolineManager::TrampolineManager(std::array<SDL_Texture*, 3> textures, float floorY,
                                     float firstX, TrampolineSettings settings)
    : textures(textures), settings(settings), floorY(floorY), firstX(firstX)
{
    double total = 0.0;
    for (const auto& tier : settings.tiers) {
        if (!std::isfinite(tier.width) || tier.width <= 0.0f || tier.width > 400.0f ||
            tier.reward < 0 || !std::isfinite(tier.weight) || tier.weight < 0.0) {
            throw std::invalid_argument("Invalid trampoline tier");
        }
        total += tier.weight;
    }
    if (!std::isfinite(total) || total <= 0.0 ||
        !std::isfinite(settings.bounceSpeed) || settings.bounceSpeed <= 0.0f) {
        throw std::invalid_argument("Invalid trampoline weights or bounce speed");
    }
}

void TrampolineManager::startRun(float x, float y, std::optional<unsigned int> seed)
{
    //remember the seed so returning to an area keeps the same platforms
    const unsigned int previousSeed = runSeed;
    if (seed) {
        runSeed = *seed;
    } else {
        std::random_device random;
        do { runSeed = random(); } while (runSeed == previousSeed);
    }
    chunks.clear();
    rewarded.clear();
    runScore = 0;
    update(x, y);
}

void TrampolineManager::update(float x, float y)
{
    const int centreX = static_cast<int>(std::floor(x / chunkWidth));
    const int centreY = static_cast<int>(std::floor(std::max(0.0f, floorY - y) / chunkHeight));
    const int firstY = std::max(0, centreY - 1);
    const std::array<double, 3> weights = {
        settings.tiers[0].weight, settings.tiers[1].weight, settings.tiers[2].weight
    };
    for (int cx = centreX - 1; cx <= centreX + 1; ++cx) {
        for (int cy = firstY; cy <= centreY + 1; ++cy) {
            const ChunkId id{cx, cy};
            if (chunks.contains(id)) {
                continue;
            }
            std::seed_seq seed{runSeed, static_cast<unsigned int>(cx),
                               static_cast<unsigned int>(cy), 0x7a4d901u};
            std::mt19937 random(seed);
            std::discrete_distribution<int> chooseTier(weights.begin(), weights.end());
            const auto tier = static_cast<std::size_t>(chooseTier(random));
            //one platform per section, spaced away from the section edges
            std::uniform_real_distribution<float> randomX(220.0f, chunkWidth - 220.0f);
            std::uniform_real_distribution<float> randomHeight(100.0f, chunkHeight - 100.0f);
            chunks.emplace(id, Trampoline{cx * chunkWidth + randomX(random),
                floorY - (cy * chunkHeight + randomHeight(random)), tier});
        }
    }
    for (auto it = chunks.begin(); it != chunks.end();) {
        if (it->first.first < centreX - 1 || it->first.first > centreX + 1 ||
            it->first.second < firstY || it->first.second > centreY + 1) {
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }
}

std::optional<TrampolineManager::Landing> TrampolineManager::land(
    SDL_FPoint previous, SDL_FPoint current, float radius)
{
    //only a downward crossing of the mat can bounce the ball
    if (current.y <= previous.y) {
        return std::nullopt;
    }
    auto selected = chunks.end();
    float earliest = 2.0f;
    float landingX = 0.0f;
    for (auto it = chunks.begin(); it != chunks.end(); ++it) {
        const auto& platform = it->second;
        if (platform.x < firstX) {
            continue;
        }
        const float t = (platform.surfaceY - radius - previous.y) / (current.y - previous.y);
        if (t < 0.0f || t > 1.0f || t >= earliest) {
            continue;
        }
        const float x = previous.x + (current.x - previous.x) * t;
        //the centre must land over the mat, excluding the outside rim
        const float halfMat = settings.tiers[platform.tier].width * 0.42f;
        if (std::abs(x - platform.x) <= halfMat) {
            earliest = t;
            landingX = x;
            selected = it;
        }
    }
    if (selected == chunks.end()) {
        return std::nullopt;
    }
    auto& platform = selected->second;
    //every landing restarts the wobble, even after the reward has been collected
    platform.bounceTime = 0.0f;
    const int reward = rewarded.insert(selected->first).second ?
                       settings.tiers[platform.tier].reward : 0;
    runScore += reward;
    return Landing{{landingX, platform.surfaceY - radius}, settings.bounceSpeed, reward};
}

void TrampolineManager::animate(float elapsed)
{
    if (!std::isfinite(elapsed) || elapsed <= 0.0f) {
        return;
    }
    for (auto& [id, platform] : chunks) {
        (void)id;
        platform.bounceTime = std::min(platform.bounceTime + elapsed, bounceDuration);
    }
}

void TrampolineManager::draw(SDL_Renderer* renderer, const Camera& camera) const
{
    for (const auto& [id, platform] : chunks) {
        (void)id;
        if (platform.x < firstX) {
            continue;
        }
        SDL_Texture* texture = textures[platform.tier];
        const float width = settings.tiers[platform.tier].width;
        //the white mat is near the top of each floating sprite
        const float height = width * (texture ? texture->h / static_cast<float>(texture->w) : 0.24f);
        //move the sprite while keeping the landing surface fixed for consistent physics
        const auto screen = camera.worldToScreen(platform.x,
            platform.surfaceY + platform.getVisualOffset());
        SDL_FRect destination{screen.x - width * 0.5f, screen.y - height * 0.15f, width, height};
        if (destination.x + width >= 0 && destination.x <= camera.getWidth() &&
            destination.y + height >= 0 && destination.y <= camera.getHeight()) {
            SDL_RenderTexture(renderer, texture, nullptr, &destination);
        }
    }
}

int TrampolineManager::getRunScore() const { return runScore; }

std::vector<TrampolineManager::Trampoline> TrampolineManager::getTrampolines() const
{
    std::vector<Trampoline> result;
    for (const auto& [id, platform] : chunks) {
        (void)id;
        if (platform.x >= firstX) {
            result.push_back(platform);
        }
    }
    return result;
}
