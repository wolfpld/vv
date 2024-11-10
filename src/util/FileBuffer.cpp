#include <format>
#include <sys/mman.h>

#include "FileBuffer.hpp"
#include "FileWrapper.hpp"
#include "util/Logs.hpp"
#include "util/Panic.hpp"

FileBuffer::FileBuffer( const char* fn )
    : m_buffer( nullptr )
    , m_size( 0 )
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

    m_buffer = (const char*)mmap( nullptr, m_size, PROT_READ, MAP_SHARED, fileno( file ), 0 );
}

FileBuffer::FileBuffer( FILE* file )
{
    CheckPanic( file, "File pointer is null" );

    fseek( file, 0, SEEK_END );
    m_size = ftell( file );
    fseek( file, 0, SEEK_SET );

    m_buffer = (const char*)mmap( nullptr, m_size, PROT_READ, MAP_SHARED, fileno( file ), 0 );
}

FileBuffer::~FileBuffer()
{
    if( m_buffer ) munmap( (void*)m_buffer, m_size );
}
