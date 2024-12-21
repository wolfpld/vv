#include <string.h>
#include <utility>

#include <stb_image_resize2.h>
#include <png.h>

#include "Alloca.h"
#include "Bitmap.hpp"
#include "Panic.hpp"

Bitmap::Bitmap( uint32_t width, uint32_t height )
    : m_width( width )
    , m_height( height )
    , m_data( new uint8_t[width*height*4] )
{
}

Bitmap::~Bitmap()
{
    delete[] m_data;
}

Bitmap::Bitmap( Bitmap&& other ) noexcept
    : m_width( other.m_width )
    , m_height( other.m_height )
    , m_data( other.m_data )
{
    other.m_data = nullptr;
}

Bitmap& Bitmap::operator=( Bitmap&& other ) noexcept
{
    std::swap( m_width, other.m_width );
    std::swap( m_height, other.m_height );
    std::swap( m_data, other.m_data );
    return *this;
}

void Bitmap::Resize( uint32_t width, uint32_t height )
{
    auto newData = new uint8_t[width*height*4];
    stbir_resize_uint8_srgb( m_data, m_width, m_height, 0, newData, width, height, 0, STBIR_RGBA );
    delete[] m_data;
    m_data = newData;
    m_width = width;
    m_height = height;
}

void Bitmap::Extend( uint32_t width, uint32_t height )
{
    CheckPanic( width >= m_width && height >= m_height, "Invalid extension" );

    auto data = new uint8_t[width*height*4];
    auto stride = width - m_width;

    auto src = m_data;
    auto dst = data;

    for( uint32_t y=0; y<m_height; y++ )
    {
        memcpy( dst, src, m_width * 4 );
        src += m_width * 4;
        dst += width * 4;
        memset( dst, 0, stride * 4 );
        dst += stride * 4;
    }
    memset( dst, 0, ( height - m_height ) * width * 4 );

    delete[] m_data;
    m_data = data;
}

void Bitmap::FlipVertical()
{
    auto ptr1 = m_data;
    auto ptr2 = m_data + ( m_height - 1 ) * m_width * 4;
    auto tmp = alloca( m_width * 4 );

    for ( uint32_t y=0; y<m_height/2; y++ )
    {
        memcpy( tmp, ptr1, m_width * 4 );
        memcpy( ptr1, ptr2, m_width * 4 );
        memcpy( ptr2, tmp, m_width * 4 );
        ptr1 += m_width * 4;
        ptr2 -= m_width * 4;
    }
}

void Bitmap::SavePng( const char* path ) const
{
    FILE* f = fopen( path, "wb" );
    CheckPanic( f, "Failed to open %s for writing", path );

    mclog( LogLevel::Info, "Saving PNG: %s", path );

    png_structp png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
    png_infop info_ptr = png_create_info_struct( png_ptr );
    setjmp( png_jmpbuf( png_ptr ) );
    png_init_io( png_ptr, f );

    png_set_IHDR( png_ptr, info_ptr, m_width, m_height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );

    png_write_info( png_ptr, info_ptr );

    auto ptr = (uint32_t*)m_data;
    for( int i=0; i<m_height; i++ )
    {
        png_write_rows( png_ptr, (png_bytepp)(&ptr), 1 );
        ptr += m_width;
    }

    png_write_end( png_ptr, info_ptr );
    png_destroy_write_struct( &png_ptr, &info_ptr );

    fclose( f );
}
