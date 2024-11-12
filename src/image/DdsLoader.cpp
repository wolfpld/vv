#include <string.h>
#include <vector>

#include "bcdec.h"
#include "DdsLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/Panic.hpp"

static void DecodeBc1Part( uint64_t d, uint32_t* dst, uint32_t w )
{
    uint8_t* in = (uint8_t*)&d;
    uint16_t c0, c1;
    uint32_t idx;
    memcpy( &c0, in, 2 );
    memcpy( &c1, in+2, 2 );
    memcpy( &idx, in+4, 4 );

    uint8_t r0 = ( ( c0 & 0xF800 ) >> 8 ) | ( ( c0 & 0xF800 ) >> 13 );
    uint8_t g0 = ( ( c0 & 0x07E0 ) >> 3 ) | ( ( c0 & 0x07E0 ) >> 9 );
    uint8_t b0 = ( ( c0 & 0x001F ) << 3 ) | ( ( c0 & 0x001F ) >> 2 );

    uint8_t r1 = ( ( c1 & 0xF800 ) >> 8 ) | ( ( c1 & 0xF800 ) >> 13 );
    uint8_t g1 = ( ( c1 & 0x07E0 ) >> 3 ) | ( ( c1 & 0x07E0 ) >> 9 );
    uint8_t b1 = ( ( c1 & 0x001F ) << 3 ) | ( ( c1 & 0x001F ) >> 2 );

    uint32_t dict[4];

    dict[0] = 0xFF000000 | ( b0 << 16 ) | ( g0 << 8 ) | r0;
    dict[1] = 0xFF000000 | ( b1 << 16 ) | ( g1 << 8 ) | r1;

    uint32_t r, g, b;
    if( c0 > c1 )
    {
        r = (2*r0+r1)/3;
        g = (2*g0+g1)/3;
        b = (2*b0+b1)/3;
        dict[2] = 0xFF000000 | ( b << 16 ) | ( g << 8 ) | r;
        r = (2*r1+r0)/3;
        g = (2*g1+g0)/3;
        b = (2*b1+b0)/3;
        dict[3] = 0xFF000000 | ( b << 16 ) | ( g << 8 ) | r;
    }
    else
    {
        r = (int(r0)+r1)/2;
        g = (int(g0)+g1)/2;
        b = (int(b0)+b1)/2;
        dict[2] = 0xFF000000 | ( b << 16 ) | ( g << 8 ) | r;
        dict[3] = 0xFF000000;
    }

    memcpy( dst+0, dict + (idx & 0x3), 4 );
    idx >>= 2;
    memcpy( dst+1, dict + (idx & 0x3), 4 );
    idx >>= 2;
    memcpy( dst+2, dict + (idx & 0x3), 4 );
    idx >>= 2;
    memcpy( dst+3, dict + (idx & 0x3), 4 );
    idx >>= 2;
    dst += w;

    memcpy( dst+0, dict + (idx & 0x3), 4 );
    idx >>= 2;
    memcpy( dst+1, dict + (idx & 0x3), 4 );
    idx >>= 2;
    memcpy( dst+2, dict + (idx & 0x3), 4 );
    idx >>= 2;
    memcpy( dst+3, dict + (idx & 0x3), 4 );
    idx >>= 2;
    dst += w;

    memcpy( dst+0, dict + (idx & 0x3), 4 );
    idx >>= 2;
    memcpy( dst+1, dict + (idx & 0x3), 4 );
    idx >>= 2;
    memcpy( dst+2, dict + (idx & 0x3), 4 );
    idx >>= 2;
    memcpy( dst+3, dict + (idx & 0x3), 4 );
    idx >>= 2;
    dst += w;

    memcpy( dst+0, dict + (idx & 0x3), 4 );
    idx >>= 2;
    memcpy( dst+1, dict + (idx & 0x3), 4 );
    idx >>= 2;
    memcpy( dst+2, dict + (idx & 0x3), 4 );
    idx >>= 2;
    memcpy( dst+3, dict + (idx & 0x3), 4 );
}

static void DecodeBc3Part( uint64_t a, uint64_t d, uint32_t* dst, uint32_t w )
{
    uint8_t* ain = (uint8_t*)&a;
    uint8_t a0, a1;
    uint64_t aidx = 0;
    memcpy( &a0, ain, 1 );
    memcpy( &a1, ain+1, 1 );
    memcpy( &aidx, ain+2, 6 );

    uint8_t* in = (uint8_t*)&d;
    uint16_t c0, c1;
    uint32_t idx;
    memcpy( &c0, in, 2 );
    memcpy( &c1, in+2, 2 );
    memcpy( &idx, in+4, 4 );

    uint32_t adict[8];
    adict[0] = a0 << 24;
    adict[1] = a1 << 24;
    if( a0 > a1 )
    {
        adict[2] = ( (6*a0+1*a1)/7 ) << 24;
        adict[3] = ( (5*a0+2*a1)/7 ) << 24;
        adict[4] = ( (4*a0+3*a1)/7 ) << 24;
        adict[5] = ( (3*a0+4*a1)/7 ) << 24;
        adict[6] = ( (2*a0+5*a1)/7 ) << 24;
        adict[7] = ( (1*a0+6*a1)/7 ) << 24;
    }
    else
    {
        adict[2] = ( (4*a0+1*a1)/5 ) << 24;
        adict[3] = ( (3*a0+2*a1)/5 ) << 24;
        adict[4] = ( (2*a0+3*a1)/5 ) << 24;
        adict[5] = ( (1*a0+4*a1)/5 ) << 24;
        adict[6] = 0;
        adict[7] = 0xFF000000;
    }

    uint8_t r0 = ( ( c0 & 0xF800 ) >> 8 ) | ( ( c0 & 0xF800 ) >> 13 );
    uint8_t g0 = ( ( c0 & 0x07E0 ) >> 3 ) | ( ( c0 & 0x07E0 ) >> 9 );
    uint8_t b0 = ( ( c0 & 0x001F ) << 3 ) | ( ( c0 & 0x001F ) >> 2 );

    uint8_t r1 = ( ( c1 & 0xF800 ) >> 8 ) | ( ( c1 & 0xF800 ) >> 13 );
    uint8_t g1 = ( ( c1 & 0x07E0 ) >> 3 ) | ( ( c1 & 0x07E0 ) >> 9 );
    uint8_t b1 = ( ( c1 & 0x001F ) << 3 ) | ( ( c1 & 0x001F ) >> 2 );

    uint32_t dict[4];

    dict[0] = ( b0 << 16 ) | ( g0 << 8 ) | r0;
    dict[1] = ( b1 << 16 ) | ( g1 << 8 ) | r1;

    uint32_t r, g, b;
    if( c0 > c1 )
    {
        r = (2*r0+r1)/3;
        g = (2*g0+g1)/3;
        b = (2*b0+b1)/3;
        dict[2] = ( b << 16 ) | ( g << 8 ) | r;
        r = (2*r1+r0)/3;
        g = (2*g1+g0)/3;
        b = (2*b1+b0)/3;
        dict[3] = ( b << 16 ) | ( g << 8 ) | r;
    }
    else
    {
        r = (int(r0)+r1)/2;
        g = (int(g0)+g1)/2;
        b = (int(b0)+b1)/2;
        dict[2] = ( b << 16 ) | ( g << 8 ) | r;
        dict[3] = 0;
    }

    dst[0] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst[1] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst[2] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst[3] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst += w;

    dst[0] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst[1] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst[2] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst[3] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst += w;

    dst[0] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst[1] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst[2] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst[3] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst += w;

    dst[0] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst[1] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst[2] = dict[idx & 0x3] | adict[aidx & 0x7];
    idx >>= 2;
    aidx >>= 3;
    dst[3] = dict[idx & 0x3] | adict[aidx & 0x7];
}

static void DecodeBc4Part( uint64_t a, uint32_t* dst, uint32_t w )
{
    uint8_t* ain = (uint8_t*)&a;
    uint8_t a0, a1;
    uint64_t aidx = 0;
    memcpy( &a0, ain, 1 );
    memcpy( &a1, ain+1, 1 );
    memcpy( &aidx, ain+2, 6 );

    uint32_t adict[8];
    adict[0] = a0;
    adict[1] = a1;
    if(a0 > a1)
    {
        adict[2] = ( (6*a0+1*a1)/7 );
        adict[3] = ( (5*a0+2*a1)/7 );
        adict[4] = ( (4*a0+3*a1)/7 );
        adict[5] = ( (3*a0+4*a1)/7 );
        adict[6] = ( (2*a0+5*a1)/7 );
        adict[7] = ( (1*a0+6*a1)/7 );
    }
    else
    {
        adict[2] = ( (4*a0+1*a1)/5 );
        adict[3] = ( (3*a0+2*a1)/5 );
        adict[4] = ( (2*a0+3*a1)/5 );
        adict[5] = ( (1*a0+4*a1)/5 );
        adict[6] = 0;
        adict[7] = 0xFF;
    }

    dst[0] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst[1] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst[2] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst[3] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst += w;

    dst[0] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst[1] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst[2] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst[3] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst += w;

    dst[0] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst[1] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst[2] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst[3] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst += w;

    dst[0] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst[1] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst[2] = adict[aidx & 0x7] | 0xFF000000;
    aidx >>= 3;
    dst[3] = adict[aidx & 0x7] | 0xFF000000;
}

static void DecodeBc5Part( uint64_t r, uint64_t g, uint32_t* dst, uint32_t w )
{
    uint8_t* rin = (uint8_t*)&r;
    uint8_t r0, r1;
    uint64_t ridx = 0;
    memcpy( &r0, rin, 1 );
    memcpy( &r1, rin+1, 1 );
    memcpy( &ridx, rin+2, 6 );

    uint8_t* gin = (uint8_t*)&g;
    uint8_t g0, g1;
    uint64_t gidx = 0;
    memcpy( &g0, gin, 1 );
    memcpy( &g1, gin+1, 1 );
    memcpy( &gidx, gin+2, 6 );

    uint32_t rdict[8];
    rdict[0] = r0;
    rdict[1] = r1;
    if(r0 > r1)
    {
        rdict[2] = ( (6*r0+1*r1)/7 );
        rdict[3] = ( (5*r0+2*r1)/7 );
        rdict[4] = ( (4*r0+3*r1)/7 );
        rdict[5] = ( (3*r0+4*r1)/7 );
        rdict[6] = ( (2*r0+5*r1)/7 );
        rdict[7] = ( (1*r0+6*r1)/7 );
    }
    else
    {
        rdict[2] = ( (4*r0+1*r1)/5 );
        rdict[3] = ( (3*r0+2*r1)/5 );
        rdict[4] = ( (2*r0+3*r1)/5 );
        rdict[5] = ( (1*r0+4*r1)/5 );
        rdict[6] = 0;
        rdict[7] = 0xFF;
    }

    uint32_t gdict[8];
    gdict[0] = g0 << 8;
    gdict[1] = g1 << 8;
    if(g0 > g1)
    {
        gdict[2] = ( (6*g0+1*g1)/7 ) << 8;
        gdict[3] = ( (5*g0+2*g1)/7 ) << 8;
        gdict[4] = ( (4*g0+3*g1)/7 ) << 8;
        gdict[5] = ( (3*g0+4*g1)/7 ) << 8;
        gdict[6] = ( (2*g0+5*g1)/7 ) << 8;
        gdict[7] = ( (1*g0+6*g1)/7 ) << 8;
    }
    else
    {
        gdict[2] = ( (4*g0+1*g1)/5 ) << 8;
        gdict[3] = ( (3*g0+2*g1)/5 ) << 8;
        gdict[4] = ( (2*g0+3*g1)/5 ) << 8;
        gdict[5] = ( (1*g0+4*g1)/5 ) << 8;
        gdict[6] = 0;
        gdict[7] = 0xFF00;
    }

    dst[0] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst[1] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst[2] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst[3] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst += w;

    dst[0] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst[1] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst[2] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst[3] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst += w;

    dst[0] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst[1] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst[2] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst[3] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst += w;

    dst[0] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst[1] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst[2] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
    dst[3] = rdict[ridx & 0x7] | gdict[gidx & 0x7] | 0xFF000000;
    ridx >>= 3;
    gidx >>= 3;
}

static void DecodeBc1( uint32_t* dst, const uint64_t* src, uint32_t width, uint32_t height )
{
    for( int y=0; y<height/4; y++ )
    {
        for( int x=0; x<width/4; x++ )
        {
            uint64_t d = *src++;
            DecodeBc1Part( d, dst, width );
            dst += 4;
        }
        dst += width * 3;
    }
}

static void DecodeBc3( uint32_t* dst, const uint64_t* src, uint32_t width, uint32_t height )
{
    for( int y=0; y<height/4; y++ )
    {
        for( int x=0; x<width/4; x++ )
        {
            uint64_t a = *src++;
            uint64_t d = *src++;
            DecodeBc3Part( a, d, dst, width );
            dst += 4;
        }
        dst += width * 3;
    }
}

static void DecodeBc4( uint32_t* dst, const uint64_t* src, uint32_t width, uint32_t height )
{
    for( int y=0; y<height/4; y++ )
    {
        for( int x=0; x<width/4; x++ )
        {
            uint64_t r = *src++;
            DecodeBc4Part( r, dst, width );
            dst += 4;
        }
        dst += width * 3;
    }
}

static void DecodeBc5( uint32_t* dst, const uint64_t* src, uint32_t width, uint32_t height )
{
    for( int y=0; y<height/4; y++ )
    {
        for( int x=0; x<width/4; x++ )
        {
            uint64_t r = *src++;
            uint64_t g = *src++;
            DecodeBc5Part( r, g, dst, width );
            dst += 4;
        }
        dst += width * 3;
    }
}

static void DecodeBc7( uint32_t* dst, const uint64_t* src, uint32_t width, uint32_t height )
{
    for( int y=0; y<height/4; y++ )
    {
        for( int x=0; x<width/4; x++ )
        {
            bcdec_bc7( src, dst, width * 4 );
            src += 2;
            dst += 4;
        }
        dst += width * 3;
    }
}

DdsLoader::DdsLoader( FileWrapper& file )
    : m_file( file )
{
    fseek( m_file, 0, SEEK_SET );
    uint32_t magic;
    m_valid = fread( &magic, 1, 4, m_file ) == 4 && magic == 0x20534444;
    if( !m_valid ) return;

    fseek( m_file, 21*4, SEEK_SET );
    fread( &m_format, 1, 4, m_file );
    switch( m_format )
    {
    case 0x31545844:    // BC1
    case 0x35545844:    // BC3
    case 0x31495441:    // BC4
    case 0x55344342:    // BC4
    case 0x32495441:    // BC5
    case 0x55354342:    // BC5
        m_valid = true;
        m_offset = 128;
        break;
    case 0x30315844:    // DX10
        fseek( m_file, 32*4, SEEK_SET );
        fread( &m_format, 1, 4, m_file );
        m_valid =
            m_format == 80 ||   // BC4
            m_format == 83 ||   // BC5
            m_format == 98;     // BC7
        m_offset = 148;
        break;
    default:
        m_valid = false;
        break;
    }
}

bool DdsLoader::IsValid() const
{
    return m_valid;
}

Bitmap* DdsLoader::Load()
{
    CheckPanic( m_valid, "Invalid DDS file" );

    fseek( m_file, 0, SEEK_END );
    const auto sz = ftell( m_file );
    fseek( m_file, 0, SEEK_SET );

    std::vector<uint8_t> buf( sz );
    fread( buf.data(), 1, sz, m_file );

    const auto ptr = (uint32_t*)buf.data();

    uint32_t width = ptr[4];
    uint32_t height = ptr[3];

    auto bmp = new Bitmap( width, height );

    switch( m_format )
    {
    case 0x31545844:
        DecodeBc1( (uint32_t*)bmp->Data(), (const uint64_t*)(buf.data() + m_offset), width, height );
        break;
    case 0x35545844:
        DecodeBc3( (uint32_t*)bmp->Data(), (const uint64_t*)(buf.data() + m_offset), width, height );
        break;
    case 0x31495441:
    case 0x55344342:
    case 80:
        DecodeBc4( (uint32_t*)bmp->Data(), (const uint64_t*)(buf.data() + m_offset), width, height );
        break;
    case 0x32495441:
    case 0x55354342:
    case 83:
        DecodeBc5( (uint32_t*)bmp->Data(), (const uint64_t*)(buf.data() + m_offset), width, height );
        break;
    case 98:
        DecodeBc7( (uint32_t*)bmp->Data(), (const uint64_t*)(buf.data() + m_offset), width, height );
        break;
    default:
        CheckPanic( false, "Unsupported DDS format" );
    }

    return bmp;
}
