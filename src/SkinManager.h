#pragma once

#include "BallSprites.h"
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <string_view>

enum class SkinSheet : std::size_t { Standard, UltraPremium };

struct SkinDefinition {
    std::string_view name;
    SkinSheet sheet;
    SDL_FRect source;
    std::int64_t price;
};

//the premium sheet has its own spacing and must not use the standard ball rectangles
inline constexpr std::array<SDL_FRect, 24> ultraPremiumSprites = {{
    {55,53,346,391}, {429,63,369,385}, {819,55,373,393},
    {1198,65,374,379}, {1582,63,376,385}, {1975,53,372,397},
    {43,483,358,358}, {429,468,366,385}, {820,473,366,379},
    {1208,485,351,357}, {1584,469,363,375}, {1978,467,367,389},
    {32,876,385,390}, {435,875,358,392}, {821,877,357,389},
    {1194,873,376,394}, {1580,884,378,376}, {1970,870,384,398},
    {37,1285,372,407}, {424,1284,376,405}, {816,1286,368,405},
    {1190,1285,384,406}, {1580,1288,378,401}, {1976,1286,370,405}
}};

class SkinManager {
public:
    static constexpr std::size_t standardCount = ballSprites.size();
    static constexpr std::size_t premiumCount = ultraPremiumSprites.size();
    static constexpr std::size_t count = standardCount + premiumCount;
    static constexpr std::size_t starter = 6;
    using Unlocks = std::bitset<count>;
    enum class Purchase { Bought, Equipped, AlreadyEquipped, NotEnoughCoins, Invalid };

    SkinManager();
    static const std::array<SkinDefinition, count>& catalog();
    bool isOwned(std::size_t id) const;
    std::size_t getEquipped() const;
    const Unlocks& getUnlocks() const;
    Purchase buyOrEquip(std::size_t id, std::int64_t& bank);
    bool restore(const Unlocks& saved, std::size_t equipped);

private:
    Unlocks unlocked;
    std::size_t equipped = starter;
};
