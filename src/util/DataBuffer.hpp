#pragma once

#include <stddef.h>

class DataBuffer
{
public:
    DataBuffer( const char* data, size_t size ) : m_data( data ), m_size( size ) {}
    virtual ~DataBuffer() = default;

    [[nodiscard]] const char* data() const { return m_data; }
    [[nodiscard]] size_t size() const { return m_size; }

protected:
    DataBuffer() : m_data( nullptr ), m_size( 0 ) {}

    const char* m_data;
    size_t m_size;
};
