#ifdef __FreeBSD__
#include <stdlib.h>
#else
#include <alloca.h>
#endif
#include <string.h>
#include <utility>

#include <stb_image_resize2.h>

#include "Bitmap.hpp"

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
