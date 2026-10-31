#include "SkinManager.h"

SkinManager::SkinManager()
{
    //preserve the original blue ball and keep the cream starter available too
    unlocked.set(0);
    unlocked.set(starter);
}

const std::array<SkinDefinition, SkinManager::count>& SkinManager::catalog()
{
    static const auto definitions = [] {
        constexpr std::array<std::string_view, standardCount> names = {
            "Cream", "Amber", "Silver", "Sunshine", "Rose", "Obsidian", "Diamond", "Magma",
            "Emerald", "Ruby", "Sapphire", "Amethyst", "Jade", "Rainbow", "Galaxy", "Aurora",
            "Slime", "Storm", "Sunburst", "Moon", "Candy", "Woodland", "Clockwork", "Stone",
            "Camo", "Checkered", "Prism", "Pearl", "Toxic", "Volcano", "Nebula", "Cloud",
            "Blossom", "Skull", "Champion", "Treasure", "Relic", "Violet", "Gold", "Rainbow Gem"
        };
        constexpr std::array<std::string_view, premiumCount> premiumNames = {
            "Royal Ruby", "Arctic Prism", "Golden Galaxy", "Rose Quartz", "Elder Nebula", "Crystal Frost",
            "Phoenix", "Sun King", "Moon Knight", "Runestone", "Chronos", "Celestial Pearl",
            "Inferno", "Rainbow Halo", "Mercury", "Void Sovereign", "Stormbound", "Tempest",
            "Emerald Crown", "Ruby Crown", "Sapphire Crown", "Amethyst Crown", "Golden Skull", "Thundercloud"
        };
        constexpr std::array<std::int64_t, 5> rowPrices{25, 100, 200, 400, 700};
        std::array<SkinDefinition, count> result{};
        for (std::size_t i = 0; i < standardCount; ++i) {
            const std::int64_t price = i == 0 || i == starter ? 0 :
                rowPrices[i / 8] + 5 * static_cast<std::int64_t>(i % 8);
            result[i] = {names[i], SkinSheet::Standard, ballSprites[i], price};
        }
        for (std::size_t i = 0; i < premiumCount; ++i) {
            result[standardCount + i] = {premiumNames[i], SkinSheet::UltraPremium,
                                         ultraPremiumSprites[i], 1200 + static_cast<std::int64_t>(i) * 100};
        }
        return result;
    }();
    return definitions;
}

bool SkinManager::isOwned(std::size_t id) const { return id < count && unlocked.test(id); }
std::size_t SkinManager::getEquipped() const { return equipped; }
const SkinManager::Unlocks& SkinManager::getUnlocks() const { return unlocked; }

SkinManager::Purchase SkinManager::buyOrEquip(std::size_t id, std::int64_t& bank)
{
    if (id >= count) return Purchase::Invalid;
    if (id == equipped) return Purchase::AlreadyEquipped;
    if (isOwned(id)) {
        equipped = id;
        return Purchase::Equipped;
    }
    const auto price = catalog()[id].price;
    if (bank < price) return Purchase::NotEnoughCoins;
    bank -= price;
    unlocked.set(id);
    equipped = id;
    return Purchase::Bought;
}

bool SkinManager::restore(const Unlocks& saved, std::size_t selected)
{
    if (selected >= count || !saved.test(selected) || !saved.test(0) || !saved.test(starter)) {
        return false;
    }
    unlocked = saved;
    equipped = selected;
    return true;
}
