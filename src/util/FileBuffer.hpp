#pragma once

#include <stddef.h>
#include <stdexcept>
#include <string>

#include "NoCopy.hpp"

class FileBuffer
{
public:
    struct FileException : public std::runtime_error { explicit FileException( const std::string& msg ) : std::runtime_error( msg ) {} };

    explicit FileBuffer( const char* fn );
    explicit FileBuffer( FILE* file );
    ~FileBuffer();

    NoCopy( FileBuffer );

    [[nodiscard]] const char* data() const { return m_buffer; }
    [[nodiscard]] size_t size() const { return m_size; }

private:
    char* m_buffer;
    size_t m_size;
};
