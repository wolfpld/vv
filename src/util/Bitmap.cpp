#include <string.h>
#include <utility>

#include <stb_image_resize2.h>
#include <png.h>

#include "Alloca.h"
#include "Bitmap.hpp"
#include "Panic.hpp"

#if defined __SSE2__
#  include <x86intrin.h>
#endif

Bitmap::Bitmap( uint32_t width, uint32_t height, int orientation )
    : m_width( width )
    , m_height( height )
    , m_data( new uint8_t[width*height*4] )
    , m_orientation( orientation )
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

    for( uint32_t y=0; y<m_height/2; y++ )
    {
        memcpy( tmp, ptr1, m_width * 4 );
        memcpy( ptr1, ptr2, m_width * 4 );
        memcpy( ptr2, tmp, m_width * 4 );
        ptr1 += m_width * 4;
        ptr2 -= m_width * 4;
    }
}

void Bitmap::FlipHorizontal()
{
    auto ptr = (uint32_t*)m_data;

    for( uint32_t y=0; y<m_height; y++ )
    {
        auto ptr1 = ptr;
        auto ptr2 = ptr + m_width - 1;

        for( uint32_t x=0; x<m_width/2; x++ )
        {
            std::swap( *ptr1++, *ptr2-- );
        }

        ptr += m_width;
    }
}

void Bitmap::Rotate90()
{
    auto tmp = new uint8_t[m_width * m_height * 4];

    auto src = (uint32_t*)m_data;
    auto dst = (uint32_t*)tmp;

    for( uint32_t y=0; y<m_height; y++ )
    {
        for( uint32_t x=0; x<m_width; x++ )
        {
            dst[x * m_height + m_height - y - 1] = src[y * m_width + x];
        }
    }

    delete[] m_data;
    m_data = tmp;
    std::swap( m_width, m_height );
}

void Bitmap::Rotate180()
{
    auto ptr1 = (uint32_t*)m_data;
    auto ptr2 = (uint32_t*)m_data + m_width * m_height - 1;

    for( uint32_t i=0; i<m_width * m_height / 2; i++ )
    {
        std::swap( *ptr1++, *ptr2-- );
    }
}

void Bitmap::Rotate270()
{
    auto tmp = new uint8_t[m_width * m_height * 4];

    auto src = (uint32_t*)m_data;
    auto dst = (uint32_t*)tmp;

    for( uint32_t y=0; y<m_height; y++ )
    {
        for( uint32_t x=0; x<m_width; x++ )
        {
            dst[( m_width - x - 1 ) * m_height + y] = src[y * m_width + x];
        }
    }

    delete[] m_data;
    m_data = tmp;
    std::swap( m_width, m_height );
}

void Bitmap::SetAlpha( uint8_t alpha )
{
    auto ptr = m_data;
    size_t sz = m_width * m_height;

    if( alpha == 0xFF )
    {
#ifdef __AVX512F__
        const auto alpha16 = _mm512_set1_epi32( alpha << 24 );
        while( sz >= 16 )
        {
            auto v = _mm512_loadu_si512( ptr );
            v = _mm512_or_si512( v, alpha16 );
            _mm512_storeu_si512( ptr, v );
            ptr += 16 * 4;
            sz -= 16;
        }
#endif
#ifdef __AVX2__
        const auto alpha8 = _mm256_set1_epi32( alpha << 24 );
        while( sz >= 8 )
        {
            auto v = _mm256_loadu_si256( (const __m256i*)ptr );
            v = _mm256_or_si256( v, alpha8 );
            _mm256_storeu_si256( (__m256i*)ptr, v );
            ptr += 8 * 4;
            sz -= 8;
        }
#endif
#ifdef __SSE2__
        const auto alpha4 = _mm_set1_epi32( alpha << 24 );
        while( sz >= 4 )
        {
            auto v = _mm_loadu_si128( (const __m128i*)ptr );
            v = _mm_or_si128( v, alpha4 );
            _mm_storeu_si128( (__m128i*)ptr, v );
            ptr += 4 * 4;
            sz -= 4;
        }
#endif
    }
    else
    {
#ifdef __AVX512F__
        const auto alpha16 = _mm512_set1_epi32( alpha << 24 );
        const auto mask16 = _mm512_set1_epi32( 0x00FFFFFF );
        while( sz >= 16 )
        {
            auto v = _mm512_loadu_si512( ptr );
            v = _mm512_and_si512( v, mask16 );
            v = _mm512_or_si512( v, alpha16 );
            _mm512_storeu_si512( ptr, v );
            ptr += 16 * 4;
            sz -= 16;
        }
#endif
#ifdef __AVX2__
        const auto alpha8 = _mm256_set1_epi32( alpha << 24 );
        const auto mask8 = _mm256_set1_epi32( 0x00FFFFFF );
        while( sz >= 8 )
        {
            auto v = _mm256_loadu_si256( (const __m256i*)ptr );
            v = _mm256_and_si256( v, mask8 );
            v = _mm256_or_si256( v, alpha8 );
            _mm256_storeu_si256( (__m256i*)ptr, v );
            ptr += 8 * 4;
            sz -= 8;
        }
#endif
#ifdef __SSE2__
        const auto alpha4 = _mm_set1_epi32( alpha << 24 );
        const auto mask4 = _mm_set1_epi32( 0x00FFFFFF );
        while( sz >= 4 )
        {
            auto v = _mm_loadu_si128( (const __m128i*)ptr );
            v = _mm_and_si128( v, mask4 );
            v = _mm_or_si128( v, alpha4 );
            _mm_storeu_si128( (__m128i*)ptr, v );
            ptr += 4 * 4;
            sz -= 4;
        }
#endif
    }

    ptr += 3;
    while( sz-- )
    {
        memset( ptr, alpha, 1 );
        ptr += 4;
    }
}

void Bitmap::NormalizeOrientation()
{
    if( m_orientation <= 1 ) return;

    switch( m_orientation )
    {
    case 2:
        FlipHorizontal();
        break;
    case 3:
        Rotate180();
        break;
    case 4:
        FlipVertical();
        break;
    case 5:
        Rotate270();
        FlipVertical();
        break;
    case 6:
        Rotate90();
        break;
    case 7:
        Rotate90();
        FlipVertical();
        break;
    case 8:
        Rotate270();
        break;
    default:
        Panic( "Invalid orientation value!" );
    }

    m_orientation = 1;
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
