#include <png.h>
#include <setjmp.h>
#include <stdio.h>

#include "PngLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/Panic.hpp"

PngLoader::PngLoader( FileWrapper& file )
    : m_file( file )
{
    fseek( m_file, 0, SEEK_SET );
    char hdr[8];
    m_valid = fread( hdr, 1, 8, m_file ) == 8 && png_sig_cmp( (png_const_bytep)hdr, 0, 8 ) == 0;
}

bool PngLoader::IsValid() const
{
    return m_valid;
}

Bitmap* PngLoader::Load()
{
    CheckPanic( m_valid, "Invalid PNG file" );

    auto png = png_create_read_struct( PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr );
    if( !png ) return nullptr;
    auto info = png_create_info_struct( png );
    if( !info )
    {
        png_destroy_read_struct( &png, nullptr, nullptr );
        return nullptr;
    }
    auto end = png_create_info_struct( png );
    if( !end )
    {
        png_destroy_read_struct( &png, &info, nullptr );
        return nullptr;
    }

    Bitmap* bmp = nullptr;

    if( setjmp( png_jmpbuf( png ) ) )
    {
        png_destroy_read_struct( &png, &info, &end );
        delete bmp;
        return nullptr;
    }

    png_init_io( png, m_file );
    png_set_sig_bytes( png, 8 );

    png_read_info( png, info );

    png_uint_32 width, height;
    int bitDepth, colorType;
    png_get_IHDR( png, info, &width, &height, &bitDepth, &colorType, nullptr, nullptr, nullptr );

    if( bitDepth == 16 ) png_set_strip_16( png );

    switch( colorType )
    {
    case PNG_COLOR_TYPE_PALETTE:
        png_set_palette_to_rgb( png );
        if( png_get_valid( png, info, PNG_INFO_tRNS ) )
        {
            png_set_tRNS_to_alpha( png );
        }
        else
        {
            png_set_filler( png, 0xFF, PNG_FILLER_AFTER );
        }
        break;
    case PNG_COLOR_TYPE_GRAY:
        if( bitDepth < 8 ) png_set_expand_gray_1_2_4_to_8( png );
        png_set_gray_to_rgb( png );
        png_set_filler( png, 0xFF, PNG_FILLER_AFTER );
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        if( bitDepth < 8 ) png_set_expand_gray_1_2_4_to_8( png );
        png_set_gray_to_rgb( png );
        break;
    case PNG_COLOR_TYPE_RGB:
        png_set_filler( png, 0xFF, PNG_FILLER_AFTER );
        break;
    default:
        break;
    }

    bmp = new Bitmap( width, height );

    auto rowPtrs = new png_bytep[height];
    auto ptr = bmp->Data();
    for( png_uint_32 i=0; i<height; i++ )
    {
        rowPtrs[i] = ptr;
        ptr += width * 4;
    }
    png_read_image( png, rowPtrs );
    delete[] rowPtrs;

    png_read_end( png, end );
    png_destroy_read_struct( &png, &info, &end );
    return bmp;
}
