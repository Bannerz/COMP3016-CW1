#include "ProgressStore.h"

#include <SDL3/SDL_filesystem.h>
#include <fstream>
#include <iomanip>
#include <limits>
#include <string>
#include <system_error>

ProgressStore::Load ProgressStore::load(const std::filesystem::path& path,
                                      UpgradeManager& upgrades, std::int64_t& bank, SkinManager& skins, EnvironmentManager& environments)
{
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        return error ? Load::Invalid : Load::Missing;
    }
    std::ifstream file(path);
    std::string header, bankLabel, levelsLabel;
    int version = 0;
    std::int64_t savedBank = 0;
    UpgradeManager::Levels levels{};
    if (!(file >> header >> version >> bankLabel >> savedBank >> levelsLabel) ||
        header != "PROTOLAUNCH" || (version < 1 || version > 3) || bankLabel != "bank" ||
        levelsLabel != "levels" || savedBank < 0 || savedBank > 1000000000000LL) {
        return Load::Invalid;
    }
    for (int& level : levels) {
        if (!(file >> level)) {
            return Load::Invalid;
        }
    }
    SkinManager loadedSkins;
    if (version >= 2) {
        std::string skinsLabel, bits, equippedLabel;
        std::size_t selected = 0;
        if (!(file >> skinsLabel >> bits >> equippedLabel >> selected) || skinsLabel != "skins" ||
            equippedLabel != "equipped" || bits.size() != SkinManager::count ||
            bits.find_first_not_of("01") != std::string::npos ||
            !loadedSkins.restore(SkinManager::Unlocks(bits), selected)) {
            return Load::Invalid;
        }
    }
    EnvironmentManager loadedEnvironments;
    if (version >= 3) {
        std::string environmentLabel, recordsLabel;
        std::size_t selected = 0;
        EnvironmentManager::Records records{};
        if (!(file >> environmentLabel >> selected >> recordsLabel) ||
            environmentLabel != "environment" || recordsLabel != "records") return Load::Invalid;
        for (auto& record : records) {
            if (!(file >> record.distance >> record.height)) return Load::Invalid;
        }
        if (!loadedEnvironments.restore(records, selected)) return Load::Invalid;
    }
    file >> std::ws;
    //validate the whole file before replacing the current progression
    UpgradeManager loadedUpgrades;
    if (!file.eof() || !loadedUpgrades.restoreLevels(levels)) {
        return Load::Invalid;
    }
    upgrades = loadedUpgrades;
    skins = loadedSkins;
    environments = loadedEnvironments;
    bank = savedBank;
    return Load::Loaded;
}

bool ProgressStore::save(const std::filesystem::path& path, const UpgradeManager& upgrades,
                         std::int64_t bank, const SkinManager& skins, const EnvironmentManager& environments)
{
    if (path.empty()) {
        return false;
    }
    auto temporary = path;
    temporary += ".tmp";
    std::ofstream file(temporary, std::ios::trunc);
    file << "PROTOLAUNCH 3\nbank " << bank << "\nlevels";
    for (int level : upgrades.getLevels()) {
        file << ' ' << level;
    }
    file << "\nskins " << skins.getUnlocks().to_string() << "\nequipped " << skins.getEquipped() << '\n';
    file << "environment " << environments.getSelected() << "\nrecords";
    //retain enough precision that a milestone remains unlocked after a reload
    file << std::setprecision(std::numeric_limits<float>::max_digits10);
    for (const auto& record : environments.getRecords()) {
        file << ' ' << record.distance << ' ' << record.height;
    }
    file << '\n';
    file.close();
    if (!file) {
        return false;
    }
    //replace only after a complete write so an interrupted save keeps the previous file
    //SDL replaces existing files on Windows too, and accepts UTF-8 paths
    const auto temporaryUtf8 = temporary.u8string();
    const auto pathUtf8 = path.u8string();
    const std::string temporaryText(temporaryUtf8.begin(), temporaryUtf8.end());
    const std::string pathText(pathUtf8.begin(), pathUtf8.end());
    if (!SDL_RenamePath(temporaryText.c_str(), pathText.c_str())) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return false;
    }
    return true;
}
