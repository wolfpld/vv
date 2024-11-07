#pragma once

#include <stdio.h>

#include "NoCopy.hpp"

class FileWrapper
{
public:
    FileWrapper( const char* fn, const char* mode )
        : m_file( fopen( fn, mode ) )
    {
    }

    ~FileWrapper()
    {
        if( m_file ) fclose( m_file );
    }

    NoCopy( FileWrapper );

    bool Read( void* dst, size_t size )
    {
        return fread( dst, 1, size, m_file ) == size;
    }

    operator bool() const { return m_file != nullptr; }
    operator FILE*() { return m_file; }

private:
    FILE* m_file;
};
