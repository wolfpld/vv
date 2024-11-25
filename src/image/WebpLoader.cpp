#include <string.h>
#include <webp/demux.h>

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

    WebPData data = {
        .bytes = (const uint8_t*)buf.data(),
        .size = buf.size()
    };

    WebPAnimDecoderOptions opts;
    WebPAnimDecoderOptionsInit( &opts );
    opts.color_mode = MODE_RGBA;
    opts.use_threads = 1;

    WebPAnimDecoder* dec = WebPAnimDecoderNew( &data, &opts );

    WebPAnimInfo info;
    WebPAnimDecoderGetInfo( dec, &info );

    int delay;
    uint8_t* out;
    if( !WebPAnimDecoderGetNext( dec, &out, &delay ) )
    {
        WebPAnimDecoderDelete( dec );
        return nullptr;
    }

    auto bmp = new Bitmap( info.canvas_width, info.canvas_height );
    memcpy( bmp->Data(), out, info.canvas_width * info.canvas_height * 4 );

    WebPAnimDecoderDelete( dec );
    return bmp;
}
