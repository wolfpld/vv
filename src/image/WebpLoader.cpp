#include <string.h>
#include <webp/demux.h>

#include "WebpLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/BitmapAnim.hpp"
#include "util/FileBuffer.hpp"
#include "util/Panic.hpp"

WebpLoader::WebpLoader( std::shared_ptr<FileWrapper> file )
    : ImageLoader( std::move( file ) )
    , m_dec( nullptr )
{
    fseek( *m_file, 0, SEEK_SET );
    uint8_t hdr[12];
    m_valid = fread( hdr, 1, 12, *m_file ) == 12 && memcmp( hdr, "RIFF", 4 ) == 0 && memcmp( hdr + 8, "WEBP", 4 ) == 0;
}

WebpLoader::~WebpLoader()
{
    if( m_dec ) WebPAnimDecoderDelete( m_dec );
}

bool WebpLoader::IsValid() const
{
    return m_valid;
}

bool WebpLoader::IsAnimated()
{
    if( !m_dec ) Open();

    WebPAnimInfo info;
    WebPAnimDecoderGetInfo( m_dec, &info );

    return info.frame_count > 1;
}

std::unique_ptr<Bitmap> WebpLoader::Load()
{
    if( !m_dec ) Open();

    WebPAnimInfo info;
    WebPAnimDecoderGetInfo( m_dec, &info );

    int delay;
    uint8_t* out;
    if( !WebPAnimDecoderGetNext( m_dec, &out, &delay ) ) return nullptr;

    auto bmp = std::make_unique<Bitmap>( info.canvas_width, info.canvas_height );
    memcpy( bmp->Data(), out, info.canvas_width * info.canvas_height * 4 );

    return bmp;
}

std::unique_ptr<BitmapAnim> WebpLoader::LoadAnim()
{
    if( !m_dec ) Open();

    WebPAnimInfo info;
    WebPAnimDecoderGetInfo( m_dec, &info );
    CheckPanic( info.frame_count > 1, "Not an animated WebP file" );

    int prevDelay = 0;
    auto anim = std::make_unique<BitmapAnim>( info.frame_count );
    for( int i=0; i<info.frame_count; i++ )
    {
        int delay;
        uint8_t* out;
        if( !WebPAnimDecoderGetNext( m_dec, &out, &delay ) ) break;

        auto bmp = std::make_shared<Bitmap>( info.canvas_width, info.canvas_height );
        memcpy( bmp->Data(), out, info.canvas_width * info.canvas_height * 4 );

        anim->AddFrame( std::move( bmp ), ( delay - prevDelay ) * 1000 );
        prevDelay = delay;
    }

    return anim;
}

void WebpLoader::Open()
{
    CheckPanic( m_valid, "Invalid WebP file" );
    CheckPanic( !m_buf && !m_dec, "Already opened" );

    m_buf = std::make_unique<FileBuffer>( m_file );

    WebPData data = {
        .bytes = (const uint8_t*)m_buf->data(),
        .size = m_buf->size()
    };

    WebPAnimDecoderOptions opts;
    WebPAnimDecoderOptionsInit( &opts );
    opts.color_mode = MODE_RGBA;
    opts.use_threads = 1;

    m_dec = WebPAnimDecoderNew( &data, &opts );
    CheckPanic( m_dec, "Failed to open WebP file" );
}
