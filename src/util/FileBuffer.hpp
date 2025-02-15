#pragma once

#include <memory>
#include <stddef.h>
#include <stdexcept>
#include <string>

#include "DataBuffer.hpp"
#include "NoCopy.hpp"

class FileWrapper;

class FileBuffer : public DataBuffer
{
public:
    struct FileException : public std::runtime_error { explicit FileException( const std::string& msg ) : std::runtime_error( msg ) {} };

    explicit FileBuffer( const char* fn );
    explicit FileBuffer( FILE* file );
    explicit FileBuffer( const std::shared_ptr<FileWrapper>& file );
    ~FileBuffer() override;

    NoCopy( FileBuffer );
};
