#include "Menu.h"

#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

Menu::Menu(SDL_Renderer* renderer, const std::string& assetRoot) : renderer(renderer)
{
    const std::string images = assetRoot + "images/";
    panel = loadNineSlice(images + "ui-panel.png", 96);
    //exclude the unused transparent space above and below the button artwork
    const SDL_FRect buttonSource{0, 40, 2172, 610};
    primaryButton = loadNineSlice(images + "ui-button-juicy-primary-master.png", 260, buttonSource);
    secondaryButton = loadNineSlice(images + "ui-button-juicy-secondary-master.png", 260, buttonSource);
    card = loadNineSlice(images + "ui-card.png", 64);
    for (const auto* filename : {"ball-sprite-sheet.png", "ultra-premium-sprite-sheet.png"}) {
        TexturePtr sheet(IMG_LoadTexture(renderer, (images + filename).c_str()), SDL_DestroyTexture);
        if (!sheet) {
            throw std::runtime_error("Skin sheet loading failed: " + std::string(SDL_GetError()));
        }
        SDL_SetTextureScaleMode(sheet.get(), SDL_SCALEMODE_LINEAR);
        skinSheets.push_back(std::move(sheet));
    }
    for (const auto& definition : UpgradeManager::definitions) {
        const std::string filename = images + "upgrades/" + std::string(definition.icon);
        TexturePtr icon(IMG_LoadTexture(renderer, filename.c_str()), SDL_DestroyTexture);
        if (!icon) {
            throw std::runtime_error("Failed to load " + filename + ": " + SDL_GetError());
        }
        SDL_SetTextureScaleMode(icon.get(), SDL_SCALEMODE_LINEAR);
        upgradeIcons.push_back(std::move(icon));
    }
    for (int size : {18, 24, 30, 58}) {
        const std::string filename = size >= 30 ? "Lilita_One/LilitaOne-Regular.ttf" :
                                                "Fredoka/static/Fredoka-Medium.ttf";
        //render text at double resolution so it stays smooth on high density screens
        FontPtr font(TTF_OpenFont((assetRoot + "fonts/" + filename).c_str(), size * 2.0f),
                     TTF_CloseFont);
        if (!font) {
            throw std::runtime_error("Menu font loading failed: " + std::string(SDL_GetError()));
        }
        fonts.emplace(size, std::move(font));
    }
}

Menu::NineSlice Menu::loadNineSlice(const std::string& filename, float sourceBorder,
                                   SDL_FRect source)
{
    NineSlice result;
    result.texture.reset(IMG_LoadTexture(renderer, filename.c_str()));
    if (!result.texture) {
        throw std::runtime_error("Failed to load " + filename + ": " + SDL_GetError());
    }
    SDL_SetTextureScaleMode(result.texture.get(), SDL_SCALEMODE_LINEAR);
    result.source = source.w > 0 ? source : SDL_FRect{0, 0,
        static_cast<float>(result.texture->w), static_cast<float>(result.texture->h)};
    result.sourceBorder = sourceBorder;
    return result;
}

void Menu::NineSlice::draw(SDL_Renderer* renderer, SDL_FRect destination,
                          float border, Uint8 brightness) const
{
    //SDL cuts one continuous image, keeping the original corners and matching joins
    //SDL rounds corner sizes up, so leave room for a positive centre on short buttons
    const float edge = std::min({border, std::max(1.0f, (destination.w - 4.0f) * 0.5f),
                                std::max(1.0f, (destination.h - 4.0f) * 0.5f)});
    SDL_SetTextureColorMod(texture.get(), brightness, brightness, brightness);
    if (!SDL_RenderTexture9Grid(renderer, texture.get(), &source,
                               sourceBorder, sourceBorder, sourceBorder, sourceBorder,
                               edge / sourceBorder, &destination)) {
        throw std::runtime_error("Menu nine-slice rendering failed: " + std::string(SDL_GetError()));
    }
}

std::vector<Menu::Button> Menu::getButtons(View view, bool hasRun,
                                          const MenuOptions& options,
                                          const UpgradeManager* upgrades, std::int64_t coinBank,
                                          const SkinManager* skins, const EnvironmentManager* environments) const
{
    if (view == View::Environments) {
        std::vector<Button> buttons;
        if (environments) {
            for (std::size_t id = 0; id < EnvironmentManager::count; ++id) {
                const bool unlocked = environments->isUnlocked(id);
                const bool chosen = environments->getSelected() == id;
                const float x = id % 2 == 0 ? 90.0f : 490.0f;
                const float y = 136.0f + static_cast<float>(id / 2) * 138.0f;
                buttons.push_back({{x + 238, y + 12, 130, 40},
                    chosen ? "Selected" : (unlocked ? "Select" : "Locked"),
                    static_cast<Action>(static_cast<int>(Action::Environment0) + static_cast<int>(id)),
                    unlocked && !chosen, unlocked && !chosen});
            }
        }
        buttons.push_back({{370, 562, 220, 40}, "Back", Action::Back, true});
        return buttons;
    }
    if (view == View::Skins) {
        std::vector<Button> buttons;
        if (skins) {
            for (std::size_t slot = 0; slot < skinsPerPage; ++slot) {
                const std::size_t id = firstSkin() + slot;
                const bool owned = skins->isOwned(id);
                const bool equipped = skins->getEquipped() == id;
                const auto cost = SkinManager::catalog().at(id).price;
                const bool enabled = !equipped && (owned || coinBank >= cost);
                const float x = 100.0f + static_cast<float>(slot % 4) * 194.0f;
                const float y = slot < 4 ? 194.0f : 366.0f;
                const auto action = static_cast<Action>(static_cast<int>(Action::Skin0) + static_cast<int>(slot));
                buttons.push_back({{x + 18, y + 112, 144, 34},
                    equipped ? "Equipped" : (owned ? "Equip" : std::to_string(cost) + " coins"),
                    action, enabled, enabled});
            }
        }
        buttons.push_back({{100, 548, 130, 48}, "Previous", Action::PreviousSkins, false, skinPage > 0});
        buttons.push_back({{730, 548, 130, 48}, "Next", Action::NextSkins, false, skinPage + 1 < skinPageCount()});
        buttons.push_back({{390, 548, 180, 48}, "Back", Action::Back, true});
        buttons.push_back({{690, 92, 170, 40}, "Upgrades", Action::Shop, false});
        buttons.push_back({{245, 140, 210, 38}, "Standard", Action::StandardSkins, !premiumSkins});
        buttons.push_back({{465, 140, 250, 38}, "Ultra premium", Action::PremiumSkins, premiumSkins});
        return buttons;
    }
    if (view == View::Shop) {
        const std::array<Action, UpgradeManager::count> actions = {
            Action::BuyLauncher, Action::BuyBands, Action::BuyMagnet,
            Action::BuyLoot, Action::BuyGlider, Action::BuyAirBoost
        };
        std::vector<Button> buttons;
        if (upgrades) {
            for (std::size_t i = 0; i < UpgradeManager::count; ++i) {
                const auto id = static_cast<UpgradeId>(i);
                const float x = i % 2 == 0 ? 90.0f : 490.0f;
                const float y = 150.0f + static_cast<float>(i / 2) * 124.0f;
                const auto cost = upgrades->getCost(id);
                const bool affordable = upgrades->canBuy(id, coinBank);
                buttons.push_back({{x + 240, y + 20, 128, 44},
                    cost > 0 ? std::to_string(cost) + " coins" : "MAX",
                    actions[i], affordable, affordable});
            }
        }
        buttons.push_back({{370, 540, 220, 56}, "Back", Action::Back, true});
        buttons.push_back({{620, 102, 230, 40}, "Skin shop", Action::Skins, false});
        return buttons;
    }
    if (view == View::Results) {
        return {
            {{203, 492, 174, 54}, "Play again", Action::Restart, true},
            {{389, 492, 174, 54}, "Shop", Action::Shop, false},
            {{575, 492, 174, 54}, "Environments", Action::Environments, false},
            {{370, 552, 220, 40}, "Main menu", Action::MainMenu, false}
        };
    }
    if (view == View::Progress) {
        return {
            {{295, 260, 370, 64}, "Save progress", Action::SaveProgress, true},
            {{295, 348, 370, 64}, "Load progress", Action::LoadProgress, false},
            {{340, 480, 280, 64}, "Back", Action::Back, false}
        };
    }
    if (view == View::Options) {
        return {
            {{295, 218, 370, 64}, std::string("Fullscreen: ") + (options.fullscreen ? "On" : "Off"), Action::Fullscreen, false},
            {{295, 300, 370, 64}, std::string("VSync: ") + (options.vsync ? "On" : "Off"), Action::VSync, false},
            {{295, 382, 370, 64}, std::string("Anti-aliasing: ") + (options.antiAliasing ? "On" : "Off"), Action::AntiAliasing, false},
            {{340, 480, 280, 64}, "Back", Action::Back, true}
        };
    }
    if (hasRun) {
        return {
            {{320, 174, 320, 44}, "Resume", Action::Play, true},
            {{320, 226, 320, 44}, "Shop", Action::Shop, false},
            {{320, 278, 320, 44}, "End run", Action::EndRun, false},
            {{320, 330, 320, 44}, "Environments", Action::Environments, false},
            {{320, 382, 320, 44}, "Save / Load", Action::Progress, false},
            {{320, 434, 320, 44}, "Options", Action::Options, false},
            {{320, 486, 320, 44}, "Quit", Action::Quit, false}
        };
    }
    return {
        {{320, 190, 320, 48}, "Play", Action::Play, true},
        {{320, 248, 320, 48}, "Shop", Action::Shop, false},
        {{320, 306, 320, 48}, "Environments", Action::Environments, false},
        {{320, 364, 320, 48}, "Save / Load", Action::Progress, false},
        {{320, 422, 320, 48}, "Options", Action::Options, false},
        {{320, 480, 320, 48}, "Quit", Action::Quit, false}
    };
}

Menu::Action Menu::handleEvent(const SDL_Event& event, View view, bool hasRun,
                               const MenuOptions& options, const UpgradeManager* upgrades,
                               std::int64_t coinBank, const SkinManager* skins, const EnvironmentManager* environments)
{
    const auto buttons = getButtons(view, hasRun, options, upgrades, coinBank, skins, environments);
    const auto findButton = [&](float x, float y) {
        const SDL_FPoint point{x, y};
        for (std::size_t i = 0; i < buttons.size(); ++i) {
            if (buttons[i].enabled && SDL_PointInRectFloat(&point, &buttons[i].bounds)) {
                return static_cast<int>(i);
            }
        }
        return -1;
    };
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        selected = findButton(event.motion.x, event.motion.y);
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        selected = findButton(event.button.x, event.button.y);
        pressed = selected;
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
        const int released = findButton(event.button.x, event.button.y);
        const int original = pressed;
        pressed = -1;
        if (released >= 0 && released == original) {
            return activate(buttons[released].action);
        }
    } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
        const int count = static_cast<int>(buttons.size());
        const bool backwards = event.key.key == SDLK_UP || event.key.key == SDLK_LEFT ||
            (event.key.key == SDLK_TAB && (event.key.mod & SDL_KMOD_SHIFT));
        if (backwards || event.key.key == SDLK_DOWN || event.key.key == SDLK_RIGHT || event.key.key == SDLK_TAB) {
            pressed = -1;
            selected = selected < 0 ? (backwards ? count - 1 : 0) :
                       (selected + (backwards ? count - 1 : 1)) % count;
            for (int attempt = 0; attempt < count && !buttons[selected].enabled; ++attempt) {
                selected = (selected + (backwards ? count - 1 : 1)) % count;
            }
        } else if ((event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) && selected >= 0 &&
                   buttons[selected].enabled) {
            return activate(buttons[selected].action);
        }
    } else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        pressed = -1;
    }
    return Action::None;
}

std::size_t Menu::firstSkin() const
{
    return (premiumSkins ? SkinManager::standardCount : 0) + skinPage * skinsPerPage;
}

std::size_t Menu::skinPageCount() const
{
    return (premiumSkins ? SkinManager::premiumCount : SkinManager::standardCount) / skinsPerPage;
}

std::size_t Menu::getSkinId(Action action) const
{
    const int slot = static_cast<int>(action) - static_cast<int>(Action::Skin0);
    if (slot < 0 || slot >= static_cast<int>(skinsPerPage)) {
        throw std::invalid_argument("Not a skin selection action");
    }
    return firstSkin() + static_cast<std::size_t>(slot);
}

Menu::Action Menu::activate(Action action)
{
    //page and category changes belong to the shop UI, rather than game state
    switch (action) {
    case Action::StandardSkins: premiumSkins = false; skinPage = 0; break;
    case Action::PremiumSkins: premiumSkins = true; skinPage = 0; break;
    case Action::PreviousSkins: if (skinPage > 0) --skinPage; break;
    case Action::NextSkins: if (skinPage + 1 < skinPageCount()) ++skinPage; break;
    default: return action;
    }
    resetInput();
    return Action::None;
}

void Menu::resetInput()
{
    selected = 0;
    pressed = -1;
}

void Menu::text(const std::string& value, float centreX, float centreY, int size,
                SDL_Color colour, bool shadow) const
{
    if (value.empty()) {
        return;
    }
    //results vary each run, so keep the cached text from growing indefinitely
    if (labels.size() >= 128) {
        labels.clear();
    }
    const std::string key = std::to_string(size) + ":" + value;
    auto found = labels.find(key);
    if (found == labels.end()) {
        //cache the label textures instead of rasterising fonts every frame
        using SurfacePtr = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;
        SurfacePtr surface(TTF_RenderText_Blended(fonts.at(size).get(), value.c_str(), value.size(),
                                                {255, 255, 255, 255}), SDL_DestroySurface);
        if (!surface) {
            throw std::runtime_error("Menu text rendering failed: " + std::string(SDL_GetError()));
        }
        TexturePtr texture(SDL_CreateTextureFromSurface(renderer, surface.get()), SDL_DestroyTexture);
        if (!texture) {
            throw std::runtime_error("Menu text texture failed: " + std::string(SDL_GetError()));
        }
        SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_LINEAR);
        found = labels.emplace(key, std::move(texture)).first;
    }
    auto* texture = found->second.get();
    const float width = texture->w * 0.5f;
    const float height = texture->h * 0.5f;
    SDL_FRect destination{centreX - width * 0.5f, centreY - height * 0.5f, width, height};
    if (shadow) {
        SDL_SetTextureColorMod(texture, 70, 32, 14);
        const SDL_FRect offset{destination.x, destination.y + 2, width, height};
        SDL_RenderTexture(renderer, texture, nullptr, &offset);
    }
    SDL_SetTextureColorMod(texture, colour.r, colour.g, colour.b);
    SDL_RenderTexture(renderer, texture, nullptr, &destination);
}

void Menu::draw(View view, bool hasRun, const MenuOptions& options,
                const std::string& message, const RunResult* result, std::int64_t coinBank,
                const UpgradeManager* upgrades, const SkinManager* skins, const EnvironmentManager* environments) const
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 16, 32, 48, 135);
    const SDL_FRect backdrop{0, 0, 960, 640};
    SDL_RenderFillRect(renderer, &backdrop);
    panel.draw(renderer, view == View::Shop || view == View::Skins || view == View::Environments ? SDL_FRect{50, 20, 860, 600} :
        (view == View::Results ? SDL_FRect{170, 20, 620, 600} : SDL_FRect{220, 30, 520, 580}), 64);
    const SDL_Color ink{77, 42, 23, 255};
    const SDL_Color muted{119, 83, 52, 255};
    const SDL_Color white{255, 249, 228, 255};
    if (view == View::Environments && environments) {
        text("ENVIRONMENTS", 480, 72, 58, ink);
        text("Reach either target in the previous area to unlock the next.", 480, 113, 18, muted);
        for (std::size_t id = 0; id < EnvironmentManager::count; ++id) {
            const auto& definition = EnvironmentManager::definitions[id];
            const auto& record = environments->getRecords()[id];
            const float x = id % 2 == 0 ? 90.0f : 490.0f;
            const float y = 136.0f + static_cast<float>(id / 2) * 138.0f;
            card.draw(renderer, {x, y, 380, 128}, 18);
            text(std::string(definition.name), x + 112, y + 30, 30, ink);
            text(std::string(definition.physicsDescription), x + 190, y + 65, 18, muted);
            std::string detail;
            if (environments->isUnlocked(id)) {
                detail = "Best: " + std::to_string(static_cast<int>(record.distance)) + " m / " +
                    std::to_string(static_cast<int>(record.height)) + " m high";
            } else {
                detail = std::string(EnvironmentManager::definitions[id - 1].name) + ": " +
                    std::to_string(static_cast<int>(definition.requiredDistance)) + " m OR " +
                    std::to_string(static_cast<int>(definition.requiredHeight)) + " m high";
            }
            text(detail, x + 190, y + 99, 18, ink);
        }
    } else if (view == View::Skins && skins) {
        text("SKIN SHOP", 480, 80, 58, ink);
        text("Bank: " + std::to_string(coinBank) + " coins  |  Unlocked: " +
             std::to_string(skins->getUnlocks().count()) + " / " + std::to_string(SkinManager::count),
             340, 116, 18, ink);
        for (std::size_t slot = 0; slot < skinsPerPage; ++slot) {
            const auto id = firstSkin() + slot;
            const auto& skin = SkinManager::catalog().at(id);
            const float x = 100.0f + static_cast<float>(slot % 4) * 194.0f;
            const float y = slot < 4 ? 194.0f : 366.0f;
            card.draw(renderer, {x, y, 180, 154}, 18);
            //fit each sprite without distorting crowns, spikes or other decorations
            const float scale = 68.0f / std::max(skin.source.w, skin.source.h);
            const SDL_FRect preview{x + 90 - skin.source.w * scale * 0.5f,
                y + 46 - skin.source.h * scale * 0.5f, skin.source.w * scale, skin.source.h * scale};
            SDL_RenderTexture(renderer, skinSheets.at(static_cast<std::size_t>(skin.sheet)).get(),
                              &skin.source, &preview);
            text(std::string(skin.name), x + 90, y + 92, 18, ink);
        }
        text("Page " + std::to_string(skinPage + 1) + " / " + std::to_string(skinPageCount()),
             480, 532, 18, muted);
    } else if (view == View::Shop && upgrades) {
        text("UPGRADE SHOP", 480, 74, 58, ink);
        text("Bank: " + std::to_string(coinBank) + " coins", 300, 123, 24, ink);
        for (std::size_t i = 0; i < UpgradeManager::count; ++i) {
            const auto id = static_cast<UpgradeId>(i);
            const auto& definition = UpgradeManager::definitions[i];
            const float x = i % 2 == 0 ? 90.0f : 490.0f;
            const float y = 150.0f + static_cast<float>(i / 2) * 124.0f;
            card.draw(renderer, {x, y, 380, 112}, 18);
            const SDL_FRect icon{x + 12, y + 12, 64, 64};
            SDL_RenderTexture(renderer, upgradeIcons.at(i).get(), nullptr, &icon);
            text(std::string(definition.name), x + 164, y + 24, 24, ink);
            text("Level " + std::to_string(upgrades->getLevel(id)) + " / " +
                 std::to_string(definition.maxLevel), x + 164, y + 52, 18, muted);
            text(std::string(definition.description), x + 190, y + 91, 18, muted);
        }
    } else if (view == View::Results && result) {
        const auto number = [](double value, int precision) {
            std::ostringstream formatted;
            formatted << std::fixed << std::setprecision(precision) << value;
            return formatted.str();
        };
        text("RUN COMPLETE", 480, 80, 58, ink);
        text("Run coins: " + std::to_string(result->baseCoins), 480, 140, 30, ink);
        const auto rewardRow = [&](const std::string& label, double multiplier, float y) {
            text(label, 370, y, 24, ink);
            text("x" + number(multiplier, 2), 650, y, 30, ink);
        };
        rewardRow("Distance: " + number(result->distance, 1) + " m", result->distanceMultiplier, 194);
        rewardRow("Max height: " + number(result->height, 1) + " m", result->heightMultiplier, 236);
        rewardRow("Run time: " + number(result->duration, 1) + " s", result->timeMultiplier, 278);
        SDL_SetRenderDrawColor(renderer, 168, 119, 42, 255);
        SDL_RenderLine(renderer, 240, 310, 720, 310);
        text("Total multiplier: x" + number(result->totalMultiplier, 2), 480, 332, 24, muted);
        primaryButton.draw(renderer, {270, 358, 420, 68}, 22);
        text("PAYOUT: +" + std::to_string(result->payout) + " COINS", 480, 391, 30, white, true);
        text("Bank: " + std::to_string(coinBank) + " coins", 480, 452, 24, ink);
    } else if (view == View::Progress) {
        text("SAVE / LOAD", 480, 120, 58, ink);
        text("Coins, upgrades, skins and areas", 480, 184, 24, ink);
        text("Runs and options are not saved.", 480, 218, 18, muted);
        text("Automatic saving is also enabled.", 480, 442, 18, muted);
    } else {
        text(view == View::Main ? "PROTOLAUNCH" : "OPTIONS", 480, 120, 58, ink);
    }

    const auto buttons = getButtons(view, hasRun, options, upgrades, coinBank, skins, environments);
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        const auto& button = buttons[i];
        SDL_FRect bounds = button.bounds;
        const bool focused = button.enabled && selected == static_cast<int>(i);
        const bool held = pressed == static_cast<int>(i) && focused;
        if (held) {
            bounds.y += 3;
        }
        (button.primary ? primaryButton : secondaryButton).draw(renderer, bounds, 22,
            !button.enabled ? 155 : (held ? 185 : (focused ? 255 : 242)));
        text(button.label, bounds.x + bounds.w * 0.5f, bounds.y + bounds.h * 0.5f - 1,
             view == View::Skins ? (i < skinsPerPage ? 18 : 24) :
             (view == View::Results ? 24 : ((view == View::Shop && i < UpgradeManager::count) || view == View::Environments ? 18 : 30)),
             button.primary ? white : ink, button.primary);
        if (focused) {
            if (view == View::Shop || view == View::Skins || view == View::Environments) {
                SDL_SetRenderDrawColor(renderer, ink.r, ink.g, ink.b, ink.a);
                const SDL_FRect outline{bounds.x - 3, bounds.y - 3, bounds.w + 6, bounds.h + 6};
                SDL_RenderRect(renderer, &outline);
            } else {
                text(">", bounds.x - 18, bounds.y + bounds.h * 0.5f, 24, ink);
            }
        }
    }
    if (view == View::Main || view == View::Options) {
        text(view == View::Options ? "" : "Mouse to aim. Release to launch.",
             480, 548 + (view == View::Options ? 22 : 0), 18, muted);
        if (view == View::Main) {
            const std::string area = environments ? std::string(EnvironmentManager::definitions[environments->getSelected()].name) : "Yard";
            text(area + " | Bank: " + std::to_string(coinBank) + " coins", 480, 575, 18, muted);
        }
    }
    const std::string hint = view == View::Environments ? "Selections apply to the next run. Unlocks are permanent." :
        view == View::Skins ? "Skins are cosmetic. Buy once, then equip for free." :
        (view == View::Shop ? "Upgrades apply to your next run" :
                             "Arrow keys / Tab to select  |  Enter to confirm");
    text(message.empty() ? hint : message,
         480, 627, 18, {255, 249, 228, 255}, true);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
