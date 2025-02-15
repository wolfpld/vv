#include <format>
#include <sys/mman.h>

#include "FileBuffer.hpp"
#include "FileWrapper.hpp"
#include "util/Logs.hpp"
#include "util/Panic.hpp"

FileBuffer::FileBuffer( const char* fn )
{
    FileWrapper file( fn, "rb" );
    if( !file )
    {
        mclog( LogLevel::Error, "Failed to open file: %s", fn );
        throw FileException( std::format( "Failed to open file: {}", fn ) );
    }

    mclog( LogLevel::Debug, "Buffering file: %s", fn );

    fseek( file, 0, SEEK_END );
    m_size = ftell( file );
    fseek( file, 0, SEEK_SET );

    m_data = (const char*)mmap( nullptr, m_size, PROT_READ, MAP_SHARED, fileno( file ), 0 );
}

FileBuffer::FileBuffer( FILE* file )
{
    CheckPanic( file, "File pointer is null" );

    fseek( file, 0, SEEK_END );
    m_size = ftell( file );
    fseek( file, 0, SEEK_SET );

    m_data = (const char*)mmap( nullptr, m_size, PROT_READ, MAP_SHARED, fileno( file ), 0 );
}

FileBuffer::FileBuffer( const std::shared_ptr<FileWrapper>& file )
    : FileBuffer( *file )
{
}

FileBuffer::~FileBuffer()
{
    if( m_data ) munmap( (void*)m_data, m_size );
}
