#include <algorithm>
#include <math.h>
#include <vector>

#include <ImfRgbaFile.h>

#include "ExrLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/Panic.hpp"

class ExrStream : public Imf::IStream
{
public:
    explicit ExrStream( FILE* f )
        : Imf::IStream( "<unknown>" )
        , m_file( f )
    {
        fseek( m_file, 0, SEEK_SET );
    }

    bool isMemoryMapped() const override { return false; }
    bool read( char c[], int n ) override { fread( c, 1, n, m_file ); return !feof( m_file ); }
    uint64_t tellg() override { return ftell( m_file ); }
    void seekg( uint64_t pos ) override { fseek( m_file, pos, SEEK_SET ); }

private:
    FILE* m_file;
};

ExrLoader::ExrLoader( FileWrapper& file )
{
    try
    {
        m_stream = std::make_unique<ExrStream>( file );
        m_exr = std::make_unique<Imf::RgbaInputFile>( *m_stream );
        m_valid = true;
    }
    catch( const std::exception& )
    {
        m_valid = false;
    }
}

ExrLoader::~ExrLoader()
{
}

bool ExrLoader::IsValid() const
{
    return m_valid;
}

namespace
{
Imf::Rgba PbrNeutral( const Imf::Rgba& hdr )
{
    constexpr auto startCompression = 0.8f - 0.04f;
    constexpr auto desaturation = 0.15f;
    constexpr auto d = 1.f - startCompression;

    const auto x = std::min( { hdr.r, hdr.g, hdr.b } );
    const auto offset = x < 0.08f ? x - 6.25f * x * x : 0.04f;

    auto color = Imf::Rgba( hdr.r - offset, hdr.g - offset, hdr.b - offset, hdr.a );

    const auto peak = std::max( { color.r, color.g, color.b } );
    if( peak < startCompression ) return color;

    const auto newPeak = 1.f - d * d / ( peak + d - startCompression );
    color.r *= newPeak / peak;
    color.g *= newPeak / peak;
    color.b *= newPeak / peak;

    const auto g = 1.f - 1.f / ( desaturation * ( peak - newPeak ) + 1.f );

    return Imf::Rgba(
        std::lerp( color.r, newPeak, g ),
        std::lerp( color.g, newPeak, g ),
        std::lerp( color.b, newPeak, g ),
        color.a
    );
}

uint32_t Tonemap( const Imf::Rgba& hdr )
{
    const auto color = PbrNeutral( hdr );

    constexpr auto gamma = 2.2f;
    constexpr auto invGamma = 1.0f / gamma;

    const auto r = std::pow( color.r, invGamma );
    const auto g = std::pow( color.g, invGamma );
    const auto b = std::pow( color.b, invGamma );

    return (uint32_t( std::clamp( b, 0.0f, 1.0f ) * 255.0f ) << 16) |
           (uint32_t( std::clamp( g, 0.0f, 1.0f ) * 255.0f ) << 8) |
            uint32_t( std::clamp( r, 0.0f, 1.0f ) * 255.0f ) |
            0xff000000;
}
}

Bitmap* ExrLoader::Load()
{
    CheckPanic( m_exr, "Invalid EXR file" );

    auto dw = m_exr->dataWindow();
    auto width = dw.max.x - dw.min.x + 1;
    auto height = dw.max.y - dw.min.y + 1;

    std::vector<Imf::Rgba> hdr;
    hdr.resize( width * height );

    m_exr->setFrameBuffer( hdr.data(), 1, width );
    m_exr->readPixels( dw.min.y, dw.max.y );

    auto bmp = new Bitmap( width, height );
    auto dst = (uint32_t*)bmp->Data();
    auto src = hdr.data();
    auto sz = width * height;
    do
    {
        *dst++ = Tonemap( *src++ );
    }
    while( --sz );

    return bmp;
}
