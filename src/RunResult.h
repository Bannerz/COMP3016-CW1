#pragma once

#include "RunStats.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

struct RunRewardSettings {
    //each interval adds 1 to its multiplier, up to the cap
    double distanceInterval = 250.0;
    double heightInterval = 100.0;
    double timeInterval = 60.0;
    double maxDistanceMultiplier = 4.0;
    double maxHeightMultiplier = 3.0;
    double maxTimeMultiplier = 2.0;
};

//freeze the payout breakdown when a run ends so it can only be awarded once
struct RunResult {
    int baseCoins = 0;
    float distance = 0;
    float height = 0;
    double duration = 0;
    double distanceMultiplier = 1;
    double heightMultiplier = 1;
    double timeMultiplier = 1;
    double totalMultiplier = 1;
    std::int64_t payout = 0;

    static RunResult calculate(const RunStats& stats, int coins, const RunRewardSettings& settings)
    {
        const auto multiplier = [](double value, double interval, double cap) {
            return std::round(std::clamp(1.0 + value / interval, 1.0, cap) * 100.0) / 100.0;
        };
        RunResult result;
        result.baseCoins = std::max(0, coins);
        result.distance = stats.getDistance();
        result.height = stats.getBestHeight();
        result.duration = stats.getDuration();
        result.distanceMultiplier = multiplier(result.distance, settings.distanceInterval,
                                                settings.maxDistanceMultiplier);
        result.heightMultiplier = multiplier(result.height, settings.heightInterval,
                                              settings.maxHeightMultiplier);
        result.timeMultiplier = multiplier(result.duration, settings.timeInterval,
                                            settings.maxTimeMultiplier);
        //use hundredths so the displayed multiplier and whole-coin payout agree exactly
        const auto hundredths = static_cast<std::int64_t>(std::llround(
            result.distanceMultiplier * result.heightMultiplier * result.timeMultiplier * 100.0));
        result.totalMultiplier = hundredths / 100.0;
        result.payout = static_cast<std::int64_t>(result.baseCoins) * hundredths / 100;
        return result;
    }
};
