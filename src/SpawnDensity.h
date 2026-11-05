#pragma once

#include <algorithm>
#include <cmath>

//retain fewer generated objects with distance, without moving existing placements
struct SpawnDensitySettings {
    float startDistance = 150.0f;
    float endDistance = 1200.0f;
    float minimumDensity = 0.30f;

    bool valid() const
    {
        return std::isfinite(startDistance) && std::isfinite(endDistance) &&
            std::isfinite(minimumDensity) && startDistance >= 0 && endDistance > startDistance &&
            minimumDensity >= 0 && minimumDensity <= 1;
    }
    float atDistance(float distance) const
    {
        const float progress = std::clamp((distance - startDistance) / (endDistance - startDistance), 0.0f, 1.0f);
        const float eased = progress * progress * (3.0f - 2.0f * progress);
        return 1.0f + (minimumDensity - 1.0f) * eased;
    }
};
