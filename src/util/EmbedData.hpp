#pragma once

#include <stddef.h>
#include <stdint.h>

#include "util/DataBuffer.hpp"

#define Unembed( name ) auto name = std::make_shared<EmbedData>( Embed::name##Size, Embed::name##Lz4Size, Embed::name##Data )

class EmbedData : public DataBuffer
{
public:
    EmbedData( size_t size, size_t lz4Size, const uint8_t* data );
    ~EmbedData() override;
};
