#include "EnvironmentManager.h"

//environment tuning: edit the named values below, then rebuild the game
//keep the order unchanged because saves use these positions as environment IDs
//distance and height are metres reached in the previous environment; either unlocks this one
//gravity is a positive magnitude in metres per second squared, applied downward
//friction controls sliding grip; rollingResistance slows rolling contact
//sky is red, green, blue, alpha; assetName selects matching ground/mountain/cloud files
//density distances are metres from the launcher; minimumDensity is the retained fraction (0 to 1)
//thinning is fixed by world position so revisiting regions does not move or remove platforms
//starColour tints the stars; starSeed selects a stable layout for each environment
//physicsDescription is display text, so update it when changing the feel of an environment
//unlock targets also validate saved progression; check existing saves after changing them
const std::array<EnvironmentDefinition, EnvironmentManager::count> EnvironmentManager::definitions = {{
    //yard
    {
        .name = "Yard",
        .assetName = "yard",
        .physicsDescription = "Normal gravity | Firm ground",
        .requiredDistance = 0.0f,
        .requiredHeight = 0.0f,
        .gravity = 9.8f,
        .friction = 0.9f,
        .rollingResistance = 0.55f,
        .sky = {135, 206, 235, 255},
        .starColour = {225, 236, 255, 255},
        .starSeed = 1,
        .coinDensity = {.startDistance = 150.0f, .endDistance = 1200.0f, .minimumDensity = 0.30f},
        .trampolineDensity = {.startDistance = 150.0f, .endDistance = 1200.0f, .minimumDensity = 0.05f}
    },
    //coast
    {
        .name = "Coast",
        .assetName = "coast",
        .physicsDescription = "Lighter gravity | Smooth ground",
        .requiredDistance = 250.0f,
        .requiredHeight = 60.0f,
        .gravity = 8.5f,
        .friction = 0.6f,
        .rollingResistance = 0.35f,
        .sky = {104, 208, 224, 255},
        .starColour = {209, 247, 255, 255},
        .starSeed = 2,
        .coinDensity = {.startDistance = 150.0f, .endDistance = 1200.0f, .minimumDensity = 0.30f},
        .trampolineDensity = {.startDistance = 150.0f, .endDistance = 1200.0f, .minimumDensity = 0.05f}
    },
    //peaks
    {
        .name = "Peaks",
        .assetName = "peaks",
        .physicsDescription = "Heavy gravity | Slippery ice",
        .requiredDistance = 600.0f,
        .requiredHeight = 180.0f,
        .gravity = 12.0f,
        .friction = 0.3f,
        .rollingResistance = 0.14f,
        .sky = {177, 205, 230, 255},
        .starColour = {235, 244, 255, 255},
        .starSeed = 3,
        .coinDensity = {.startDistance = 150.0f, .endDistance = 1200.0f, .minimumDensity = 0.30f},
        .trampolineDensity = {.startDistance = 150.0f, .endDistance = 1200.0f, .minimumDensity = 0.05f}
    },
    //storm
    {
        .name = "Storm",
        .assetName = "storm",
        .physicsDescription = "Strong gravity | Grippy ground",
        .requiredDistance = 1000.0f,
        .requiredHeight = 350.0f,
        .gravity = 10.8f,
        .friction = 0.8f,
        .rollingResistance = 0.45f,
        .sky = {84, 101, 137, 255},
        .starColour = {222, 220, 255, 255},
        .starSeed = 4,
        .coinDensity = {.startDistance = 150.0f, .endDistance = 1200.0f, .minimumDensity = 0.30f},
        .trampolineDensity = {.startDistance = 150.0f, .endDistance = 1200.0f, .minimumDensity = 0.05f}
    },
    //orbit
    {
        .name = "Orbit",
        .assetName = "orbit",
        .physicsDescription = "Low gravity | Long rolls",
        .requiredDistance = 1600.0f,
        .requiredHeight = 600.0f,
        .gravity = 3.2f,
        .friction = 0.45f,
        .rollingResistance = 0.20f,
        .sky = {44, 48, 93, 255},
        .starColour = {255, 232, 209, 255},
        .starSeed = 5,
        .coinDensity = {.startDistance = 150.0f, .endDistance = 1200.0f, .minimumDensity = 0.30f},
        .trampolineDensity = {.startDistance = 150.0f, .endDistance = 1200.0f, .minimumDensity = 0.05f}
    },
    //void
    {
        .name = "Void",
        .assetName = "void",
        .physicsDescription = "Light gravity | Slick ground",
        .requiredDistance = 2500.0f,
        .requiredHeight = 1000.0f,
        .gravity = 5.5f,
        .friction = 0.2f,
        .rollingResistance = 0.12f,
        .sky = {66, 32, 93, 255},
        .starColour = {235, 201, 255, 255},
        .starSeed = 6,
        .coinDensity = {.startDistance = 150.0f, .endDistance = 1200.0f, .minimumDensity = 0.30f},
        .trampolineDensity = {.startDistance = 150.0f, .endDistance = 1200.0f, .minimumDensity = 0.05f}
    },
}};
