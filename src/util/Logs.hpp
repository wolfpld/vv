#pragma once

#include <stddef.h>

enum class LogLevel
{
    Callstack,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

void SetLogLevel( LogLevel level );
void SetLogSynchronized( bool sync );
void SetLogToFile( bool enabled );

LogLevel GetLogLevel();

void LogBlockBegin();
void LogBlockEnd();

void MCoreLogMessage( LogLevel level, const char* fileName, size_t line, const char* fmt, ... );

#define mclog(level, fmt, ...) MCoreLogMessage( level, __FILE__, __LINE__, fmt, ##__VA_ARGS__ )
