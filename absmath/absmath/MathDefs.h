#pragma once

namespace Math {

static constexpr float M_EPSILON = 0.000001f;
static constexpr float M_TWOPI = static_cast<float>(M_PI) * 2.0f;      // 2 * PI
static constexpr float M_PIF = static_cast<float>(M_PI);               // PI

/// Intersection test result.
enum Intersection
{
    OUTSIDE,
    INTERSECTS,
    INSIDE
};

}