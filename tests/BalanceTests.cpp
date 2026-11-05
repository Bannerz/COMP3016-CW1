#include "EnvironmentManager.h"
#include "RunResult.h"
#include "TrampolineManager.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

void checkPlatforms()
{
    const std::array<int, 3> rewards{3, 7, 18};
    for (std::size_t tier = 0; tier < rewards.size(); ++tier) {
        TrampolineSettings settings;
        settings.density.minimumDensity = 1.0f;
        for (std::size_t i = 0; i < settings.tiers.size(); ++i)
            settings.tiers[i].weight = i == tier ? 1.0 : 0.0;
        TrampolineManager manager({nullptr, nullptr, nullptr}, 640.0f, 0.0f, settings);
        manager.startRun(140.0f, 500.0f, 42);
        const auto platforms = manager.getTrampolines();
        require(!platforms.empty(), "missing test platform");
        const auto platform = platforms.front();
        const auto land = [&] {
            return manager.land({platform.x, platform.surfaceY - 32.0f},
                                {platform.x, platform.surfaceY - 12.0f}, 22.0f);
        };
        require(!manager.land({platform.x, platform.surfaceY - 12.0f},
                              {platform.x, platform.surfaceY - 32.0f}, 22.0f),
                "ascending ball triggered a bounce");
        const std::array<float, 4> speeds{1400.0f, 1050.0f, 700.0f, 700.0f};
        for (std::size_t i = 0; i < speeds.size(); ++i) {
            const auto landing = land();
            require(landing && landing->upwardSpeed == speeds[i], "wrong repeat thrust");
            require(landing->reward == (i == 0 ? rewards[tier] : 0), "wrong one-time reward");
        }
        //unload the platform and return to its region
        manager.update(platform.x + 100000.0f, platform.surfaceY);
        manager.update(platform.x, platform.surfaceY);
        const auto revisited = land();
        require(revisited && revisited->upwardSpeed == 700.0f && revisited->reward == 0,
                "unloading restored thrust or reward");
        manager.startRun(140.0f, 500.0f, 42);
        const auto restarted = land();
        require(restarted && restarted->upwardSpeed == 1400.0f && restarted->reward == rewards[tier],
                "new run did not restore thrust and reward");
    }
    TrampolineSettings invalid;
    invalid.bounceMultipliers[1] = 1.1f;
    bool rejected = false;
    try {
        TrampolineManager manager({nullptr, nullptr, nullptr}, 640.0f, 0.0f, invalid);
    } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "increasing repeat thrust accepted");
}

void checkRewardsAndEnvironments()
{
    RunStats stats(40.0f);
    stats.reset(140.0f);
    const auto empty = RunResult::calculate(stats, 0, {});
    require(empty.payout == 0 && empty.totalMultiplier == 1.0, "wrong empty payout");
    stats.update({140.0f + 400.0f * 40.0f, 640.0f - 22.0f - 150.0f * 40.0f},
                 640.0f, 22.0f, 60.0);
    const auto normal = RunResult::calculate(stats, 100, {});
    require(normal.payout == 500 && normal.totalMultiplier == 5.0, "wrong normal payout");
    stats.update({400000.0f, -400000.0f}, 640.0f, 22.0f, 10000.0);
    const auto capped = RunResult::calculate(stats, 100, {});
    require(capped.payout == 938 && std::abs(capped.totalMultiplier - 9.38) < 0.00001,
            "wrong rounded payout cap");
    EnvironmentManager environments;
    const std::array<float, 6> resistance{0.55f, 0.35f, 0.14f, 0.45f, 0.20f, 0.12f};
    for (std::size_t i = 0; i < resistance.size(); ++i) {
        const auto level = environments.makeLevel(i);
        require(level.groundRollingResistance == resistance[i], "wrong environment resistance");
        for (const auto& density : {level.coinDensity, level.trampolines.density}) {
            require(density.valid() && density.atDistance(150.0f) == 1.0f,
                    "invalid early density");
            require(std::abs(density.atDistance(1200.0f) - density.minimumDensity) < 0.00001f,
                    "wrong late density");
            float previous = 1.0f;
            for (float distance = 0; distance < 2000; distance += 10) {
                const float current = density.atDistance(distance);
                require(current <= previous + 0.00001f, "density increased with distance");
                previous = current;
            }
        }
        require(level.launchPower == 20.0f && level.maxDragDistance == 100.0f,
                "starting launch changed");
        require(level.coinDensity.minimumDensity == 0.30f &&
                level.trampolines.density.minimumDensity == 0.05f, "wrong density floors");
    }
}
}

int main()
{
    try {
        checkPlatforms();
        checkRewardsAndEnvironments();
        std::cout << "Balance checks passed: repeat bounces, streaming/reset history, payouts and six environments.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
