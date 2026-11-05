#pragma once

#include "Preferences.h"
#include <filesystem>

//persist options independently so loading or resetting a game keeps user preferences
class PreferencesStore {
public:
    enum class Load { Missing, Loaded, Invalid };
    static Load load(const std::filesystem::path& path, MenuOptions& options);
    static bool save(const std::filesystem::path& path, const MenuOptions& options);
};
