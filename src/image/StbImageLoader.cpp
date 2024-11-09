#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_JPEG
#define STBI_NO_PNG
#include <stb_image.h>

#include "StbImageLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/Panic.hpp"

StbImageLoader::StbImageLoader( FileWrapper& file )
    : m_file( file )
{
    fseek( m_file, 0, SEEK_SET );
    int w, h, comp;
     m_valid = stbi_info_from_file( m_file, &w, &h, &comp ) == 1;
}

bool StbImageLoader::IsValid() const
{
    return m_valid;
}

Bitmap* StbImageLoader::Load()
{
    CheckPanic( m_valid, "Invalid stb_image file" );

    fseek( m_file, 0, SEEK_SET );

    int w, h, comp;
    auto data = stbi_load_from_file( m_file, &w, &h, &comp, 4 );
    if( data == nullptr ) return nullptr;

    auto bmp = new Bitmap( w, h );
    memcpy( bmp->Data(), data, w * h * 4 );

    stbi_image_free( data );
    return bmp;
}
