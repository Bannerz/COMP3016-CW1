#pragma once

#include "Background.h"
#include "TrampolineManager.h"
#include "RunResult.h"
#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include <cstddef>
#include <array>
#include <string>

//settings for the current level, ready for more levels later
struct LevelConfig {
    std::string floorImage = "game-ground-yard.png";
    std::string launcherImage = "game-launcher-level-00.png";
    std::string ballImage = "ball-sprite-sheet.png";
    std::string premiumBallImage = "ultra-premium-sprite-sheet.png";
    std::array<std::string, 3> coinImages = {
        "game-coin-1.png", "game-coin-2.png", "game-coin-3.png"
    };
    std::string coinIconImage = "game-coin.png";
    std::array<std::string, 3> trampolineImages = {
        "game-trampoline-floating-green.png", "game-trampoline-floating-blue.png",
        "game-trampoline-floating-purple.png"
    };
    TrampolineSettings trampolines;
    RunRewardSettings runRewards;
    //relative chances for 1, 2 and 3 point coins
    std::array<double, 3> coinValueWeights = {60.0, 28.0, 12.0};
    std::string mountainImage = "game-mountains-yard.png";
    std::string cloudImage = "game-cloud-yard.png";
    BackgroundSettings background;
    b2Vec2 gravity = {0.0f, -9.8f};
    float launcherX = 140.0f;
    //start at 50 metres per second with a full pull, leaving room for upgrades
    float launchPower = 20.0f;
    float maxDragDistance = 100.0f;
    //surface resistance changes the ground behaviour without adding air drag
    float groundFriction = 0.9f;
    //keep this below the sliding friction so contact does not lock the spin
    float groundRollingResistance = 0.4f;
    float ballHeight = 44.0f;
    std::size_t ballSpriteIndex = 6;
};
