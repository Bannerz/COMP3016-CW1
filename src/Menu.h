#pragma once

#include "RunResult.h"
#include "EnvironmentManager.h"
#include "UpgradeManager.h"
#include "SkinManager.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct MenuOptions {
    bool fullscreen = false;
    bool vsync = false;
    bool antiAliasing = true;
};

//draw the menus from reusable slices and keep menu input away from the launcher
class Menu {
public:
    enum class View { Main, Options, Results, Shop, Skins, Progress, Environments };
    enum class Action { None, Play, Restart, Options, Quit, Back,
                        Fullscreen, VSync, AntiAliasing, EndRun, MainMenu, Shop,
                        BuyLauncher, BuyBands, BuyMagnet, BuyLoot, BuyGlider, BuyAirBoost,
                        Progress, SaveProgress, LoadProgress,
                        Environments, Environment0, Environment1, Environment2, Environment3, Environment4, Environment5,
                        Skins, StandardSkins, PremiumSkins, PreviousSkins, NextSkins,
                        Skin0, Skin1, Skin2, Skin3, Skin4, Skin5, Skin6, Skin7 };

    Menu(SDL_Renderer* renderer, const std::string& assetRoot);
    Action handleEvent(const SDL_Event& event, View view, bool hasRun,
                       const MenuOptions& options, const UpgradeManager* upgrades = nullptr,
                       std::int64_t coinBank = 0, const SkinManager* skins = nullptr,
                       const EnvironmentManager* environments = nullptr);
    void draw(View view, bool hasRun, const MenuOptions& options,
              const std::string& message, const RunResult* result = nullptr,
              std::int64_t coinBank = 0, const UpgradeManager* upgrades = nullptr,
              const SkinManager* skins = nullptr,
              const EnvironmentManager* environments = nullptr) const;
    void resetInput();
    std::size_t getSkinId(Action action) const;

private:
    using TexturePtr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
    using FontPtr = std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)>;
    struct NineSlice {
        TexturePtr texture{nullptr, SDL_DestroyTexture};
        SDL_FRect source{};
        float sourceBorder = 0;
        void draw(SDL_Renderer* renderer, SDL_FRect destination, float border,
                  Uint8 brightness = 255) const;
    };
    struct Button {
        SDL_FRect bounds;
        std::string label;
        Action action;
        bool primary;
        bool enabled = true;
    };

    NineSlice loadNineSlice(const std::string& filename, float sourceBorder,
                           SDL_FRect source = {});
    std::vector<Button> getButtons(View view, bool hasRun, const MenuOptions& options,
                                   const UpgradeManager* upgrades, std::int64_t coinBank,
                                   const SkinManager* skins, const EnvironmentManager* environments) const;
    Action activate(Action action);
    std::size_t firstSkin() const;
    std::size_t skinPageCount() const;
    void text(const std::string& value, float centreX, float centreY, int size,
              SDL_Color colour, bool shadow = false) const;

    SDL_Renderer* renderer;
    NineSlice panel;
    NineSlice primaryButton;
    NineSlice secondaryButton;
    NineSlice card;
    std::vector<TexturePtr> upgradeIcons;
    std::vector<TexturePtr> skinSheets;
    std::map<int, FontPtr> fonts;
    mutable std::map<std::string, TexturePtr> labels;
    int selected = 0;
    int pressed = -1;
    std::size_t skinPage = 0;
    bool premiumSkins = false;
    static constexpr std::size_t skinsPerPage = 8;
};
