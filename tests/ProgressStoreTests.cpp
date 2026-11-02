#include "ProgressStore.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* description)
{
    if (!condition) throw std::runtime_error(description);
}
}

int main(int argc, char** argv)
{
    try {
        require(argc == 2, "Supply a directory for test saves");
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto directory = std::filesystem::path(argv[1]) / std::to_string(stamp);
        std::filesystem::create_directories(directory);
        const auto namedSave = directory / std::filesystem::path(u8"caf\u00e9 - first save.dat");
        const auto otherSave = directory / "second-save.dat";
        const auto autosave = directory / "progress.dat";
        UpgradeManager upgrades;
        require(upgrades.restoreLevels({2, 3, 1, 2, 4, 1}), "Fixture upgrade levels");
        SkinManager skins;
        std::int64_t bank = 10000;
        require(skins.buyOrEquip(1, bank) == SkinManager::Purchase::Bought, "Fixture skin purchase");
        EnvironmentManager environments;
        environments.recordRun(0, 300.0f, 80.0f);
        require(environments.select(1), "Fixture environment unlock");
        require(ProgressStore::save(namedSave, upgrades, bank, skins, environments), "Save a named file with a Unicode path");
        require(ProgressStore::save(otherSave, upgrades, 123, skins, environments), "Save an independent second file");
        require(ProgressStore::save(autosave, upgrades, bank, skins, environments), "Create autosave");
        const auto verifyOriginal = [&](const std::filesystem::path& path, std::int64_t expectedBank) {
            UpgradeManager loadedUpgrades;
            SkinManager loadedSkins;
            EnvironmentManager loadedEnvironments;
            std::int64_t loadedBank = 0;
            require(ProgressStore::load(path, loadedUpgrades, loadedBank, loadedSkins, loadedEnvironments) == ProgressStore::Load::Loaded, "Load saved progression");
            require(loadedBank == expectedBank && loadedUpgrades.getLevels() == upgrades.getLevels(), "Restore bank and every upgrade");
            require(loadedSkins.getUnlocks() == skins.getUnlocks() && loadedSkins.getEquipped() == skins.getEquipped(), "Restore skins and equipment");
            require(loadedEnvironments.getSelected() == 1 && loadedEnvironments.getRecords()[0].distance == 300.0f &&
                    loadedEnvironments.getRecords()[0].height == 80.0f, "Restore environment selection and records");
        };
        verifyOriginal(namedSave, bank);
        verifyOriginal(otherSave, 123);
        const auto firstBackup = ProgressStore::backup(directory, upgrades, bank, skins, environments);
        const auto secondBackup = ProgressStore::backup(directory, upgrades, bank, skins, environments);
        require(firstBackup && secondBackup && *firstBackup != *secondBackup, "Backups have unique paths");
        //The fresh-game sequence replaces autosave only after a successful backup.
        require(ProgressStore::save(autosave, UpgradeManager{}, 0, SkinManager{}, EnvironmentManager{}), "Write fresh autosave");
        verifyOriginal(namedSave, bank);
        verifyOriginal(otherSave, 123);
        verifyOriginal(*firstBackup, bank);
        verifyOriginal(*secondBackup, bank);
        UpgradeManager freshUpgrades;
        SkinManager freshSkins;
        EnvironmentManager freshEnvironments;
        std::int64_t freshBank = -1;
        require(ProgressStore::load(autosave, freshUpgrades, freshBank, freshSkins, freshEnvironments) == ProgressStore::Load::Loaded &&
                freshBank == 0 && freshUpgrades.getLevels() == UpgradeManager{}.getLevels() &&
                freshSkins.getUnlocks() == SkinManager{}.getUnlocks() && freshEnvironments.getSelected() == 0,
                "Fresh autosave contains starter progression");
        const auto invalid = directory / "invalid.dat";
        { std::ofstream file(invalid); file << "PROTOLAUNCH 3\nbank 900\nlevels 1 2"; }
        const auto expectedLevels = upgrades.getLevels();
        const auto expectedSkins = skins.getUnlocks();
        const auto expectedBank = bank;
        require(ProgressStore::load(invalid, upgrades, bank, skins, environments) == ProgressStore::Load::Invalid,
                "Reject truncated save");
        require(bank == expectedBank && upgrades.getLevels() == expectedLevels && skins.getUnlocks() == expectedSkins &&
                environments.getSelected() == 1, "Invalid load leaves the session untouched");
        require(ProgressStore::load(directory / "missing.dat", upgrades, bank, skins, environments) == ProgressStore::Load::Missing && bank == expectedBank,
                "Missing load leaves the session untouched");
        require(!ProgressStore::backup(directory / "nonexistent" / "folder", upgrades, bank, skins, environments), "Report backup failure");
        verifyOriginal(namedSave, bank);
        std::cout << "Passed: multiple saves, Unicode paths, complete round trips, unique backups, fresh progression, failure preservation.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
