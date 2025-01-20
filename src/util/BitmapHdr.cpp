#include <cmath>

#include "Bitmap.hpp"
#include "BitmapHdr.hpp"

BitmapHdr::BitmapHdr( uint32_t width, uint32_t height )
    : m_width( width )
    , m_height( height )
    , m_data( new float[width*height*4] )
{
}

BitmapHdr::~BitmapHdr()
{
    delete[] m_data;
}

std::unique_ptr<Bitmap> BitmapHdr::Tonemap( ToneMap::Operator op )
{
    auto bmp = std::make_unique<Bitmap>( m_width, m_height );
    ToneMap::Process( op, (uint32_t*)bmp->Data(), m_data, m_width * m_height );
    return bmp;
}
