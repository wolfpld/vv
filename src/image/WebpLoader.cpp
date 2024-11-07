#include <string.h>
#include <webp/decode.h>
#include <vector>

#include "WebpLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/Panic.hpp"

WebpLoader::WebpLoader( FileWrapper& file )
    : m_file( file )
{
    fseek( m_file, 0, SEEK_SET );
    uint8_t hdr[12];
    m_valid = fread( hdr, 1, 12, m_file ) == 12 && memcmp( hdr, "RIFF", 4 ) == 0 && memcmp( hdr + 8, "WEBP", 4 ) == 0;
}

bool WebpLoader::IsValid() const
{
    return m_valid;
}

Bitmap* WebpLoader::Load()
{
    CheckPanic( m_valid, "Invalid WebP file" );

    fseek( m_file, 0, SEEK_END );
    const auto sz = ftell( m_file );
    fseek( m_file, 0, SEEK_SET );

    std::vector<uint8_t> buf( sz );
    fread( buf.data(), 1, sz, m_file );

    int width, height;
    if( !WebPGetInfo( buf.data(), buf.size(), &width, &height ) ) return nullptr;

    auto bmp = new Bitmap( width, height );

    if( WebPDecodeRGBAInto( buf.data(), buf.size(), bmp->Data(), bmp->Width() * bmp->Height() * 4, bmp->Width() * 4 ) == nullptr )
    {
        delete bmp;
        return nullptr;
    }

    return bmp;
}
