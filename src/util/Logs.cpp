#include <assert.h>
#include <mutex>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <tracy/Tracy.hpp>

#include "Ansi.hpp"
#include "Callstack.hpp"
#include "Logs.hpp"

namespace
{
LogLevel s_logLevel = LogLevel::Info;
bool s_logSynchronized = false;
FILE* s_logFile = nullptr;
TracyLockableN( std::recursive_mutex, s_logLock, "Logger" );
}

void SetLogLevel( LogLevel level )
{
    s_logLevel = level;
}

void SetLogSynchronized( bool sync )
{
    s_logSynchronized = sync;
}

void SetLogToFile( bool enabled )
{
    std::lock_guard lock( s_logLock );
    if( enabled )
    {
        assert( !s_logFile );
        s_logFile = fopen( "mcore.log", "w" );
    }
    else
    {
        assert( s_logFile );
        fclose( s_logFile );
        s_logFile = nullptr;
    }
}

LogLevel GetLogLevel()
{
    return s_logLevel;
}

void LogBlockBegin()
{
    if( s_logSynchronized ) s_logLock.lock();
}

void LogBlockEnd()
{
    if( s_logSynchronized ) s_logLock.unlock();
}

namespace
{
void PrintLevel( LogLevel level )
{
    const char* str = nullptr;

    switch( level )
    {
    case LogLevel::Callstack: str = ANSI_CYAN "[STACK] "; break;
    case LogLevel::Debug: str = ANSI_BOLD ANSI_BLACK "[DEBUG] "; break;
    case LogLevel::Info: str = " [INFO] "; break;
    case LogLevel::Warning: str = ANSI_BOLD ANSI_YELLOW " [WARN] "; break;
    case LogLevel::Error: str = ANSI_BOLD ANSI_RED "[ERROR] "; break;
    case LogLevel::Fatal: str = ANSI_BOLD ANSI_MAGENTA "[FATAL] "; break;
    default: assert( false ); break;
    }

    printf( "%s", str );
    if( s_logFile ) fprintf( s_logFile, "%s", str );
}

void PrintSourceLocation( FILE* f, const char* fileName, size_t len, size_t line )
{
    constexpr int FnLen = 20;
    if( len > FnLen )
    {
        fprintf( f, "…%s:%-4zu│ ", fileName + len - FnLen - 1, line );
    }
    else
    {
        fprintf( f, "%*s:%-4zu%*s│ ", FnLen, fileName, line, int( FnLen - len + 2 ), "" );
    }
}
}

void MCoreLogMessage( LogLevel level, const char* fileName, size_t line, const char *fmt, ... )
{
    if( level < s_logLevel ) return;

    va_list args;
    va_start( args, fmt );
    const auto len = strlen( fileName );

#ifndef DISABLE_CALLSTACK
    // Get callstack outside of lock
    const bool printCallstack = level >= LogLevel::Error && s_logLevel <= LogLevel::Callstack;
    CallstackData stack;
    if( printCallstack ) stack.count = backtrace( stack.addr, 64 );
#endif

    s_logLock.lock();
    PrintLevel( level );
    PrintSourceLocation( stdout, fileName, len, line );
    vprintf( fmt, args );
    printf( ANSI_RESET "\n" );
    fflush( stdout );
    if( s_logFile )
    {
        va_end( args );
        va_start( args, fmt );
        PrintSourceLocation( s_logFile, fileName, len, line );
        vfprintf( s_logFile, fmt, args );
        fprintf( s_logFile, ANSI_RESET "\n" );
        fflush( s_logFile );
    }
#ifndef DISABLE_CALLSTACK
    if( printCallstack ) PrintCallstack( stack, 1 );
#endif
    s_logLock.unlock();
    va_end( args );

#ifdef TRACY_ENABLE
    if( level != LogLevel::Callstack )
    {
        va_start( args, fmt );
        char tmp[8*1024];
        const auto res = vsnprintf( tmp, sizeof( tmp ), fmt, args );
        if( res > 0 )
        {
            switch( level )
            {
            case LogLevel::Debug: TracyMessageC( tmp, res, 0x888888 ); break;
            case LogLevel::Info: TracyMessage( tmp, res ); break;
            case LogLevel::Warning: TracyMessageC( tmp, res, 0xFFFF00 ); break;
            case LogLevel::Error: TracyMessageCS( tmp, res, 0xFF0000, 64 ); break;
            case LogLevel::Fatal: TracyMessageCS( tmp, res, 0xFF00FF, 64 ); break;
            default: assert( false ); break;
            }
        }
        va_end( args );
    }
#endif
}
