#include <string.h>
#include <webp/decode.h>

#include "WebpLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/FileBuffer.hpp"
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

    FileBuffer buf( m_file );

    int width, height;
    if( !WebPGetInfo( (const uint8_t*)buf.data(), buf.size(), &width, &height ) ) return nullptr;

    auto bmp = new Bitmap( width, height );

    if( WebPDecodeRGBAInto( (const uint8_t*)buf.data(), buf.size(), bmp->Data(), bmp->Width() * bmp->Height() * 4, bmp->Width() * 4 ) == nullptr )
    {
        delete bmp;
        return nullptr;
    }

    return bmp;
}
