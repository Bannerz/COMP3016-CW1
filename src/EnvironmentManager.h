#pragma once

#include "LevelConfig.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <cmath>
#include <string_view>

struct EnvironmentDefinition {
    std::string_view name;
    std::string_view assetName;
    std::string_view physicsDescription;
    float requiredDistance;
    float requiredHeight;
    float gravity;
    float friction;
    float rollingResistance;
    SDL_Color sky;
};

//keep each environments records so unlocks remain available when revisiting earlier areas
class EnvironmentManager {
public:
    static constexpr std::size_t count = 6;
    struct Record { float distance = 0; float height = 0; };
    using Records = std::array<Record, count>;
    inline static constexpr std::array<EnvironmentDefinition, count> definitions = {{
        {"Yard", "yard", "Normal gravity | Firm ground", 0, 0, 9.8f, 0.9f, 0.4f, {135, 206, 235, 255}},
        {"Coast", "coast", "Lighter gravity | Smooth ground", 250, 60, 8.5f, 0.6f, 0.25f, {104, 208, 224, 255}},
        {"Peaks", "peaks", "Heavy gravity | Slippery ice", 600, 180, 12.0f, 0.3f, 0.08f, {177, 205, 230, 255}},
        {"Storm", "storm", "Strong gravity | Grippy ground", 1000, 350, 10.8f, 0.8f, 0.3f, {84, 101, 137, 255}},
        {"Orbit", "orbit", "Low gravity | Long rolls", 1600, 600, 3.2f, 0.45f, 0.12f, {44, 48, 93, 255}},
        {"Void", "void", "Light gravity | Slick ground", 2500, 1000, 5.5f, 0.2f, 0.06f, {66, 32, 93, 255}}
    }};

    bool isUnlocked(std::size_t id) const
    {
        if (id >= count) return false;
        if (id == 0) return true;
        const auto& target = definitions[id];
        return isUnlocked(id - 1) && (records[id - 1].distance >= target.requiredDistance ||
                                      records[id - 1].height >= target.requiredHeight);
    }
    bool select(std::size_t id)
    {
        if (!isUnlocked(id)) return false;
        selected = id;
        return true;
    }
    std::size_t getSelected() const { return selected; }
    const Records& getRecords() const { return records; }
    bool recordRun(std::size_t id, float distance, float height)
    {
        if (!isUnlocked(id) || !valid(distance) || !valid(height)) return false;
        records[id].distance = std::max(records[id].distance, distance);
        records[id].height = std::max(records[id].height, height);
        return true;
    }
    bool restore(const Records& saved, std::size_t choice)
    {
        EnvironmentManager candidate;
        candidate.records = saved;
        for (std::size_t i = 0; i < count; ++i) {
            if (!valid(saved[i].distance) || !valid(saved[i].height) ||
                (!candidate.isUnlocked(i) && (saved[i].distance > 0 || saved[i].height > 0))) return false;
        }
        if (!candidate.select(choice)) return false;
        *this = candidate;
        return true;
    }
    LevelConfig makeLevel(std::size_t id) const
    {
        const auto& definition = definitions.at(id);
        LevelConfig level;
        const std::string suffix(definition.assetName);
        level.floorImage = "game-ground-" + suffix + ".png";
        level.mountainImage = "game-mountains-" + suffix + ".png";
        level.cloudImage = "game-cloud-" + suffix + ".png";
        level.gravity.y = -definition.gravity;
        level.groundFriction = definition.friction;
        level.groundRollingResistance = definition.rollingResistance;
        level.background.skyAtGround = definition.sky;
        return level;
    }

private:
    static bool valid(float value) { return std::isfinite(value) && value >= 0 && value <= 1000000000.0f; }
    Records records{};
    std::size_t selected = 0;
};
