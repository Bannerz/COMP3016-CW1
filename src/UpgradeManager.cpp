#include "UpgradeManager.h"

int UpgradeManager::getLevel(UpgradeId id) const
{
    return levels.at(static_cast<std::size_t>(id));
}

std::int64_t UpgradeManager::getCost(UpgradeId id) const
{
    const auto& definition = definitions.at(static_cast<std::size_t>(id));
    const int level = getLevel(id);
    return level >= definition.maxLevel ? 0 : definition.baseCost * (std::int64_t{1} << level);
}

bool UpgradeManager::canBuy(UpgradeId id, std::int64_t bank) const
{
    const auto cost = getCost(id);
    return cost > 0 && bank >= cost;
}

UpgradeManager::Purchase UpgradeManager::buy(UpgradeId id, std::int64_t& bank)
{
    const auto cost = getCost(id);
    if (cost == 0) {
        return Purchase::MaxLevel;
    }
    if (bank < cost) {
        return Purchase::NotEnoughCoins;
    }
    bank -= cost;
    ++levels.at(static_cast<std::size_t>(id));
    return Purchase::Bought;
}

UpgradeEffects UpgradeManager::getEffects() const
{
    UpgradeEffects effects;
    effects.launchMultiplier += 0.08f * getLevel(UpgradeId::Launcher);
    effects.pullBonus = 8.0f * getLevel(UpgradeId::Bands);
    effects.magnetRange = 20.0f * getLevel(UpgradeId::Magnet);
    effects.gravityScale -= 0.06f * getLevel(UpgradeId::Glider);
    effects.boostCharges = getLevel(UpgradeId::AirBoost);
    effects.coinWeightMultipliers[1] += 0.15 * getLevel(UpgradeId::LuckyLoot);
    effects.coinWeightMultipliers[2] += 0.4 * getLevel(UpgradeId::LuckyLoot);
    return effects;
}

const UpgradeManager::Levels& UpgradeManager::getLevels() const { return levels; }

bool UpgradeManager::restoreLevels(const Levels& saved)
{
    for (std::size_t i = 0; i < count; ++i) {
        if (saved[i] < 0 || saved[i] > definitions[i].maxLevel) {
            return false;
        }
    }
    levels = saved;
    return true;
}
