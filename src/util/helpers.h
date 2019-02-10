#pragma once

#include "core/angle.h"
#include "core/vec.h"

#include "gsl-lite.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace util
{
template<typename T>
T clamp(const T& v, const T& min, const T& max)
{
    if( v < min )
    {
        return min;
    }
    if( max < v )
    {
        return max;
    }
    return v;
}

template<typename T>
constexpr auto square(T value)
{
    return value * value;
}

inline int16_t rand15()
{
    return gsl::narrow_cast<int16_t>( std::rand() & ((1 << 15) - 1) );
}

template<typename T>
inline T rand15(T max)
{
    return max * rand15() / (1 << 15);
}

template<typename T, typename U>
inline T rand15(T max, const U&)
{
    return max * U( rand15() ) / U( 1 << 15 );
}

inline int16_t rand15s()
{
    return static_cast<int16_t>(rand15() - (1 << 14));
}

template<typename T, typename U>
inline T rand15s(T max, const U& = int16_t{})
{
    return max * U( rand15s() ) / U( 1 << 15 );
}

template<typename T>
inline T rand15s(T max)
{
    return max * rand15s() / (1 << 15);
}

constexpr float auToDeg(const int16_t au)
{
    return au / 65536.0f * 360;
}

inline float auToRad(const int16_t au)
{
    return au / 65536.0f * 2 * glm::pi<float>();
}

inline glm::mat4 mix(const glm::mat4& a, const glm::mat4& b, const float bias)
{
    glm::mat4 result{0.0f};
    const auto ap = value_ptr( a );
    const auto bp = value_ptr( b );
    const auto rp = value_ptr( result );
    for( int i = 0; i < 16; ++i )
        rp[i] = ap[i] * (1 - bias) + bp[i] * bias;
    return result;
}

inline core::TRVec rotateY(core::Angle angle, core::Length x, core::Length y, core::Length z)
{
    const auto sin = angle.sin();
    const auto cos = angle.cos();
    return core::TRVec{
            (z.retype_as<float>() * sin + x.retype_as<float>() * cos).retype_as<core::Length>(),
            y,
            (z.retype_as<float>() * cos - x.retype_as<float>() * sin).retype_as<core::Length>()
    };
}
}
