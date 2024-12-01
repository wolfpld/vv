#pragma once

#include <memory>
#include <stddef.h>
#include <stdexcept>
#include <string>

#include "NoCopy.hpp"

class FileWrapper;

class FileBuffer
{
public:
    struct FileException : public std::runtime_error { explicit FileException( const std::string& msg ) : std::runtime_error( msg ) {} };

    explicit FileBuffer( const char* fn );
    explicit FileBuffer( FILE* file );
    explicit FileBuffer( const std::shared_ptr<FileWrapper>& file );
    ~FileBuffer();

    NoCopy( FileBuffer );

    [[nodiscard]] const char* data() const { return m_buffer; }
    [[nodiscard]] size_t size() const { return m_size; }

private:
    const char* m_buffer;
    size_t m_size;
};
