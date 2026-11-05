#include "PreferencesStore.h"

#include <SDL3/SDL_filesystem.h>
#include <fstream>
#include <string>
#include <system_error>

PreferencesStore::Load PreferencesStore::load(const std::filesystem::path& path, MenuOptions& options)
{
    std::error_code error;
    if (!std::filesystem::exists(path, error)) return error ? Load::Invalid : Load::Missing;
    std::ifstream file(path);
    std::string header, label;
    int version = 0;
    if (!(file >> header >> version) || header != "PROTOLAUNCH_PREFERENCES" || version != 1) return Load::Invalid;
    MenuOptions candidate;
    const auto readToggle = [&](const char* expected, bool& toggle) {
        int value = -1;
        if (!(file >> label >> value) || label != expected || (value != 0 && value != 1)) return false;
        toggle = value == 1;
        return true;
    };
    const auto readVolume = [&](const char* expected, int& volume) {
        return (file >> label >> volume) && label == expected && volume >= 0 && volume <= 100;
    };
    if (!readToggle("fullscreen", candidate.fullscreen) || !readToggle("vsync", candidate.vsync) ||
        !readToggle("antiAliasing", candidate.antiAliasing) || !readVolume("musicVolume", candidate.musicVolume) ||
        !readVolume("effectsVolume", candidate.effectsVolume)) return Load::Invalid;
    file >> std::ws;
    if (!file.eof()) return Load::Invalid;
    //apply only after every field has been validated
    options = candidate;
    return Load::Loaded;
}

bool PreferencesStore::save(const std::filesystem::path& path, const MenuOptions& options)
{
    if (path.empty() || options.musicVolume < 0 || options.musicVolume > 100 ||
        options.effectsVolume < 0 || options.effectsVolume > 100) return false;
    auto temporary = path;
    temporary += ".tmp";
    std::ofstream file(temporary, std::ios::trunc);
    file << "PROTOLAUNCH_PREFERENCES 1\nfullscreen " << options.fullscreen
         << "\nvsync " << options.vsync << "\nantiAliasing " << options.antiAliasing
         << "\nmusicVolume " << options.musicVolume << "\neffectsVolume " << options.effectsVolume << '\n';
    file.close();
    //replace the old file only after the complete write succeeds, including on Windows
    const auto temporaryUtf8 = temporary.u8string();
    const auto pathUtf8 = path.u8string();
    const std::string temporaryText(temporaryUtf8.begin(), temporaryUtf8.end());
    const std::string pathText(pathUtf8.begin(), pathUtf8.end());
    if (file && SDL_RenamePath(temporaryText.c_str(), pathText.c_str())) return true;
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    return false;
}
