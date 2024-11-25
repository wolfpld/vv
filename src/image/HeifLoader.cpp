#include <libheif/heif.h>
#include <string.h>

#include "HeifLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/FileBuffer.hpp"
#include "util/Panic.hpp"

HeifLoader::HeifLoader( FileWrapper& file )
    : m_file( file )
    , m_valid( false )
{
    fseek( m_file, 0, SEEK_SET );
    uint8_t hdr[12];
    if( fread( hdr, 1, 12, m_file ) == 12 )
    {
        const auto res = heif_check_filetype( hdr, 12 );
        m_valid = res == heif_filetype_yes_supported || res == heif_filetype_maybe;
    }
}

bool HeifLoader::IsValid() const
{
    return m_valid;
}

Bitmap* HeifLoader::Load()
{
    CheckPanic( m_valid, "Invalid HEIF file" );

    FileBuffer buf( m_file );

    auto ctx = heif_context_alloc();
    auto err = heif_context_read_from_memory_without_copy( ctx, buf.data(), buf.size(), nullptr );
    if( err.code != heif_error_Ok )
    {
        heif_context_free( ctx );
        return nullptr;
    }

    heif_image_handle* handle;
    err = heif_context_get_primary_image_handle( ctx, &handle );
    if( err.code != heif_error_Ok )
    {
        heif_context_free( ctx );
        return nullptr;
    }

    heif_image* img;
    err = heif_decode_image( handle, &img, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, nullptr );
    if( err.code != heif_error_Ok )
    {
        heif_image_handle_release( handle );
        heif_context_free( ctx );
        return nullptr;
    }

    const auto w = heif_image_get_primary_width( img );
    const auto h = heif_image_get_primary_height( img );

    int stride;
    auto src = heif_image_get_plane_readonly( img, heif_channel_interleaved, &stride );
    if( !src )
    {
        heif_image_release( img );
        heif_image_handle_release( handle );
        heif_context_free( ctx );
        return nullptr;
    }

    auto bmp = new Bitmap( w, h );
    if( stride == w * 4 )
    {
        memcpy( bmp->Data(), src, w * h * 4 );
    }
    else
    {
        auto dst = bmp->Data();
        for( int i=0; i<h; i++ )
        {
            memcpy( dst, src, w * 4 );
            dst += w * 4;
            src += stride;
        }
    }

    heif_image_release( img );
    heif_image_handle_release( handle );
    heif_context_free( ctx );
    return bmp;
}
