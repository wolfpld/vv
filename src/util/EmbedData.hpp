#pragma once

#include <stddef.h>
#include <stdint.h>

class EmbedData
{
public:
    EmbedData( size_t size, size_t lz4Size, const uint8_t* data );
    ~EmbedData();

    size_t size() const { return m_size; }
    const uint8_t* data() const { return m_data; }

private:
    size_t m_size;
    uint8_t* m_data;
};
