#include <algorithm>
#include <cmath>

#include "Bitmap.hpp"
#include "BitmapHdr.hpp"

BitmapHdr::BitmapHdr( uint32_t width, uint32_t height )
    : m_width( width )
    , m_height( height )
    , m_data( new float[width*height*4] )
{
}

BitmapHdr::~BitmapHdr()
{
    delete[] m_data;
}

namespace
{
struct HdrColor
{
    float r;
    float g;
    float b;
};

HdrColor PbrNeutral( const HdrColor& hdr )
{
    constexpr auto startCompression = 0.8f - 0.04f;
    constexpr auto desaturation = 0.15f;
    constexpr auto d = 1.f - startCompression;

    const auto x = std::min( { hdr.r, hdr.g, hdr.b } );
    const auto offset = x < 0.08f ? x - 6.25f * x * x : 0.04f;

    auto color = HdrColor { hdr.r - offset, hdr.g - offset, hdr.b - offset };

    const auto peak = std::max( { color.r, color.g, color.b } );
    if( peak < startCompression ) return color;

    const auto newPeak = 1.f - d * d / ( peak + d - startCompression );
    color.r *= newPeak / peak;
    color.g *= newPeak / peak;
    color.b *= newPeak / peak;

    const auto g = 1.f - 1.f / ( desaturation * ( peak - newPeak ) + 1.f );

    return {
        std::lerp( color.r, newPeak, g ),
        std::lerp( color.g, newPeak, g ),
        std::lerp( color.b, newPeak, g ),
    };
}
}

std::unique_ptr<Bitmap> BitmapHdr::Tonemap()
{
    auto bmp = std::make_unique<Bitmap>( m_width, m_height );
    auto dst = (uint32_t*)bmp->Data();
    auto src = m_data;
    auto sz = m_width * m_height;

    do
    {
        constexpr auto gamma = 2.2f;
        constexpr auto invGamma = 1.0f / gamma;

        const auto color = PbrNeutral( { src[0], src[1], src[2] } );

        const auto r = std::pow( color.r, invGamma );
        const auto g = std::pow( color.g, invGamma );
        const auto b = std::pow( color.b, invGamma );
        const auto a = src[3];

        *dst++ = (uint32_t( std::clamp( a, 0.0f, 1.0f ) * 255.0f ) << 24) |
                 (uint32_t( std::clamp( b, 0.0f, 1.0f ) * 255.0f ) << 16) |
                 (uint32_t( std::clamp( g, 0.0f, 1.0f ) * 255.0f ) << 8) |
                  uint32_t( std::clamp( r, 0.0f, 1.0f ) * 255.0f );

        src += 4;
    }
    while( --sz );

    return bmp;
}
