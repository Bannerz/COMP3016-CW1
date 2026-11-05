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
    SDL_Color starColour = {225, 236, 255, 255};
    Uint32 starSeed = 1;
    SpawnDensitySettings coinDensity;
    SpawnDensitySettings trampolineDensity{150.0f, 1200.0f, 0.05f};
};

//keep each environments records so unlocks remain available when revisiting earlier areas
class EnvironmentManager {
public:
    static constexpr std::size_t count = 6;
    struct Record { float distance = 0; float height = 0; };
    using Records = std::array<Record, count>;
    //editable environment values live in EnvironmentSettings.cpp
    static const std::array<EnvironmentDefinition, count> definitions;

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
        level.background.starColour = definition.starColour;
        level.background.starSeed = definition.starSeed;
        level.coinDensity = definition.coinDensity;
        level.trampolines.density = definition.trampolineDensity;
        return level;
    }

private:
    static bool valid(float value) { return std::isfinite(value) && value >= 0 && value <= 1000000000.0f; }
    Records records{};
    std::size_t selected = 0;
};
