// Based on https://github.com/bWFuanVzYWth/AgX/blob/main/agx.glsl

#include <algorithm>
#include <array>
#include <cmath>

#include "Tonemapper.hpp"

namespace ToneMap
{

float AgxCurve( float x )
{
    constexpr auto threshold = 0.6060606060606061f;
    constexpr auto a_up = 69.86278913545539f;
    constexpr auto a_down = 59.507875f;
    constexpr auto b_up = 13.0f / 4.0f;
    constexpr auto b_down = 3.0f;
    constexpr auto c_up = -4.0f / 13.0f;
    constexpr auto c_down = -1.0f / 3.0f;

    const float mask = x < threshold ? 0.f : 1.f;
    const float a = a_up + (a_down - a_up) * mask;
    const float b = b_up + (b_down - b_up) * mask;
    const float c = c_up + (c_down - c_up) * mask;

    return 0.5f + ( -2.f * threshold + 2.f * x ) * pow( 1.f + a * pow( fabs( x - threshold ), b ), c );
}

HdrColor AgxTransform( const HdrColor& hdr )
{
    constexpr auto min_ev = -12.473931188332413f;
    constexpr auto max_ev = 4.026068811667588f;
    constexpr auto range = max_ev - min_ev;
    constexpr auto invrange = 1.f / range;

    constexpr std::array agx_mat = {
        0.8424010709504686f, 0.04240107095046854f, 0.04240107095046854f,
        0.07843650156180276f, 0.8784365015618028f, 0.07843650156180276f,
        0.0791624274877287f, 0.0791624274877287f, 0.8791624274877287f
    };

    const HdrColor c1 = {
        agx_mat[0] * hdr.r + agx_mat[1] * hdr.g + agx_mat[2] * hdr.b,
        agx_mat[3] * hdr.r + agx_mat[4] * hdr.g + agx_mat[5] * hdr.b,
        agx_mat[6] * hdr.r + agx_mat[7] * hdr.g + agx_mat[8] * hdr.b,
        hdr.a
    };

    const HdrColor c2 = {
        c1.r <= 0 ? 0.f : std::clamp( std::log2( c1.r ) * invrange - min_ev * invrange, 0.f, 1.f ),
        c1.g <= 0 ? 0.f : std::clamp( std::log2( c1.g ) * invrange - min_ev * invrange, 0.f, 1.f ),
        c1.b <= 0 ? 0.f : std::clamp( std::log2( c1.b ) * invrange - min_ev * invrange, 0.f, 1.f ),
        c1.a
    };

    const HdrColor c3 = {
        AgxCurve( c2.r ),
        AgxCurve( c2.g ),
        AgxCurve( c2.b ),
        c2.a
    };

    return c3;
}

HdrColor AgxLookGolden( const HdrColor& color )
{
    const auto luma = 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
    const auto vr = std::pow( std::max( 0.f, color.r ), 0.8f );
    const auto vg = std::pow( std::max( 0.f, color.g * 0.9f ), 0.8f );
    const auto vb = std::pow( std::max( 0.f, color.b * 0.5f ), 0.8f );
    return HdrColor {
        luma + 0.8f * ( vr - luma ),
        luma + 0.8f * ( vg - luma ),
        luma + 0.8f * ( vb - luma ),
        color.a
    };
}

HdrColor AgxLookPunchy( const HdrColor& color )
{
    const auto luma = 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
    const auto vr = std::pow( std::max( 0.f, color.r ), 1.35f );
    const auto vg = std::pow( std::max( 0.f, color.g ), 1.35f );
    const auto vb = std::pow( std::max( 0.f, color.b ), 1.35f );
    return HdrColor {
        luma + 1.4f * ( vr - luma ),
        luma + 1.4f * ( vg - luma ),
        luma + 1.4f * ( vb - luma ),
        color.a
    };
}

HdrColor AgxEotf( const HdrColor& color )
{
    constexpr std::array agx_mat_inv = {
        1.1969986613119143f, -0.053001338688085674f, -0.053001338688085674f,
        -0.09804562695225345f, 1.1519543730477466f, -0.09804562695225345f,
        -0.09895303435966087f, -0.09895303435966087f, 1.151046965640339f
    };

    const HdrColor out = {
        agx_mat_inv[0] * color.r + agx_mat_inv[1] * color.g + agx_mat_inv[2] * color.b,
        agx_mat_inv[3] * color.r + agx_mat_inv[4] * color.g + agx_mat_inv[5] * color.b,
        agx_mat_inv[6] * color.r + agx_mat_inv[7] * color.g + agx_mat_inv[8] * color.b,
        color.a
    };

    return HdrColor {
        std::pow( std::max( 0.f, out.r ), 2.2f ),
        std::pow( std::max( 0.f, out.g ), 2.2f ),
        std::pow( std::max( 0.f, out.b ), 2.2f ),
        out.a
    };
}

void AgX( uint32_t* dst, float* src, size_t sz )
{
    do
    {
        auto color = AgxTransform( { src[0], src[1], src[2] } );
        color = AgxEotf( color );

        const auto r = LinearToSrgb( color.r );
        const auto g = LinearToSrgb( color.g );
        const auto b = LinearToSrgb( color.b );
        const auto a = src[3];

        *dst++ = (uint32_t( std::clamp( a, 0.0f, 1.0f ) * 255.0f ) << 24) |
                 (uint32_t( std::clamp( b, 0.0f, 1.0f ) * 255.0f ) << 16) |
                 (uint32_t( std::clamp( g, 0.0f, 1.0f ) * 255.0f ) << 8) |
                  uint32_t( std::clamp( r, 0.0f, 1.0f ) * 255.0f );

        src += 4;
    }
    while( --sz );
}

void AgXGolden( uint32_t* dst, float* src, size_t sz )
{
    do
    {
        auto color = AgxTransform( { src[0], src[1], src[2] } );
        color = AgxLookGolden( color );
        color = AgxEotf( color );

        const auto r = LinearToSrgb( color.r );
        const auto g = LinearToSrgb( color.g );
        const auto b = LinearToSrgb( color.b );
        const auto a = src[3];

        *dst++ = (uint32_t( std::clamp( a, 0.0f, 1.0f ) * 255.0f ) << 24) |
                 (uint32_t( std::clamp( b, 0.0f, 1.0f ) * 255.0f ) << 16) |
                 (uint32_t( std::clamp( g, 0.0f, 1.0f ) * 255.0f ) << 8) |
                  uint32_t( std::clamp( r, 0.0f, 1.0f ) * 255.0f );

        src += 4;
    }
    while( --sz );
}

void AgXPunchy( uint32_t* dst, float* src, size_t sz )
{
    do
    {
        auto color = AgxTransform( { src[0], src[1], src[2] } );
        color = AgxLookPunchy( color );
        color = AgxEotf( color );

        const auto r = LinearToSrgb( color.r );
        const auto g = LinearToSrgb( color.g );
        const auto b = LinearToSrgb( color.b );
        const auto a = src[3];

        *dst++ = (uint32_t( std::clamp( a, 0.0f, 1.0f ) * 255.0f ) << 24) |
                 (uint32_t( std::clamp( b, 0.0f, 1.0f ) * 255.0f ) << 16) |
                 (uint32_t( std::clamp( g, 0.0f, 1.0f ) * 255.0f ) << 8) |
                  uint32_t( std::clamp( r, 0.0f, 1.0f ) * 255.0f );

        src += 4;
    }
    while( --sz );
}

}
