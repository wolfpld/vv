#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_JPEG
#define STBI_NO_PNG
#include <stb_image.h>

#include "StbImageLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/BitmapHdr.hpp"
#include "util/FileWrapper.hpp"
#include "util/Panic.hpp"

StbImageLoader::StbImageLoader( std::shared_ptr<FileWrapper> file )
    : ImageLoader( std::move( file ) )
{
    fseek( *m_file, 0, SEEK_SET );
    int w, h, comp;
    m_valid = stbi_info_from_file( *m_file, &w, &h, &comp ) == 1;
    m_hdr = stbi_is_hdr_from_file( *m_file );
}

bool StbImageLoader::IsValid() const
{
    return m_valid;
}

bool StbImageLoader::IsHdr()
{
    return m_hdr;
}

bool StbImageLoader::PreferHdr()
{
    return true;
}

std::unique_ptr<Bitmap> StbImageLoader::Load()
{
    CheckPanic( m_valid, "Invalid stb_image file" );

    fseek( *m_file, 0, SEEK_SET );

    int w, h, comp;
    auto data = stbi_load_from_file( *m_file, &w, &h, &comp, 4 );
    if( data == nullptr ) return nullptr;

    auto bmp = std::make_unique<Bitmap>( w, h );
    memcpy( bmp->Data(), data, w * h * 4 );

    stbi_image_free( data );
    return bmp;
}

std::unique_ptr<BitmapHdr> StbImageLoader::LoadHdr()
{
    CheckPanic( m_valid, "Invalid stb_image file" );
    if( !m_hdr ) return nullptr;

    fseek( *m_file, 0, SEEK_SET );

    int w, h, comp;
    auto data = stbi_loadf_from_file( *m_file, &w, &h, &comp, 4 );
    if( data == nullptr ) return nullptr;

    auto hdr = std::make_unique<BitmapHdr>( w, h );
    memcpy( hdr->Data(), data, w * h * 4 * sizeof( float ) );

    stbi_image_free( data );
    return hdr;
}
