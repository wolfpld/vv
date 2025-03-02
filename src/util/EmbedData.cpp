#include <lz4.h>

#include "EmbedData.hpp"

EmbedData::EmbedData( size_t size, size_t lz4Size, const uint8_t* data )
    : DataBuffer( new char[size], size )
{
    LZ4_decompress_safe( (const char*)data, (char*)m_data, lz4Size, size );
}

EmbedData::~EmbedData()
{
    delete[] m_data;
}
