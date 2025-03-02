#include <png.h>
#include <setjmp.h>
#include <string.h>

#include "PngLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/FileBuffer.hpp"
#include "util/FileWrapper.hpp"
#include "util/Panic.hpp"

PngLoader::PngLoader( const std::shared_ptr<FileWrapper>& file )
    : PngLoader( std::make_shared<FileBuffer>( file ) )
{
}

PngLoader::PngLoader( std::shared_ptr<DataBuffer> buf )
{
    if( buf->size() < 8 ) return;
    if( png_sig_cmp( (png_const_bytep)buf->data(), 0, 8 ) != 0 ) return;
    m_buf = std::move( buf );
    m_offset = 8;
}

bool PngLoader::IsValid() const
{
    return m_buf != nullptr;
}

std::unique_ptr<Bitmap> PngLoader::Load()
{
    CheckPanic( m_buf, "Invalid PNG file" );

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

    if( setjmp( png_jmpbuf( png ) ) )
    {
        png_destroy_read_struct( &png, &info, &end );
        return nullptr;
    }

    png_set_read_fn( png, this, []( png_structp png, png_bytep data, png_size_t length )
    {
        auto self = (PngLoader*)png_get_io_ptr( png );
        if( self->m_offset + length > self->m_buf->size() ) png_error( png, "Read error" );
        memcpy( data, self->m_buf->data() + self->m_offset, length );
        self->m_offset += length;
    } );
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

    auto bmp = std::make_unique<Bitmap>( width, height );

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
