#pragma once

//application preferences are separate from portable game progression
struct MenuOptions {
    bool fullscreen = false;
    bool vsync = true;
    bool antiAliasing = true;
    int musicVolume = 100;
    int effectsVolume = 100;
    bool operator==(const MenuOptions&) const = default;
};
