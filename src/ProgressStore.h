#pragma once

#include "UpgradeManager.h"
#include "SkinManager.h"
#include "EnvironmentManager.h"
#include <filesystem>

//store progression separately from the assets so rebuilding does not erase purchases
class ProgressStore {
public:
    enum class Load { Missing, Loaded, Invalid };
    static Load load(const std::filesystem::path& path, UpgradeManager& upgrades, std::int64_t& bank,
                     SkinManager& skins, EnvironmentManager& environments);
    static bool save(const std::filesystem::path& path, const UpgradeManager& upgrades, std::int64_t bank,
                     const SkinManager& skins, const EnvironmentManager& environments);
};
