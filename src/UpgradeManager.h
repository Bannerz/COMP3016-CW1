#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

enum class UpgradeId : std::size_t { Launcher, Bands, Magnet, LuckyLoot, Glider, AirBoost, Count };

struct UpgradeDefinition {
    std::string_view name;
    std::string_view description;
    std::string_view icon;
    std::int64_t baseCost;
    int maxLevel = 5;
};

struct UpgradeEffects {
    float launchMultiplier = 1;
    float pullBonus = 0;
    float magnetRange = 0;
    float gravityScale = 1;
    int boostCharges = 0;
    std::array<double, 3> coinWeightMultipliers{1, 1, 1};
};

//own upgrade levels, pricing and the effects applied at the start of each run
class UpgradeManager {
public:
    static constexpr std::size_t count = static_cast<std::size_t>(UpgradeId::Count);
    using Levels = std::array<int, count>;
    inline static constexpr std::array<UpgradeDefinition, count> definitions = {{
        {"Launcher", "+8% launch power per level", "launcher.png", 25},
        {"Bands", "+8 px pull distance per level", "bands.png", 20},
        {"Coin Magnet", "+20 px pickup reach per level", "magnet.png", 15},
        {"Lucky Loot", "More 2 and 3 value coins", "luckyloot.png", 30},
        {"Glider", "6% less gravity per level", "glider.png", 40},
        {"Air Boost", "+1 boost per run / Up arrow", "airboost.png", 50}
    }};
    enum class Purchase { Bought, NotEnoughCoins, MaxLevel };

    int getLevel(UpgradeId id) const;
    std::int64_t getCost(UpgradeId id) const;
    bool canBuy(UpgradeId id, std::int64_t bank) const;
    Purchase buy(UpgradeId id, std::int64_t& bank);
    UpgradeEffects getEffects() const;
    const Levels& getLevels() const;
    bool restoreLevels(const Levels& saved);

private:
    Levels levels{};
};
