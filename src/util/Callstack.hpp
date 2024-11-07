#pragma once

#include <execinfo.h>

struct CallstackData
{
    void* addr[64];
    int count;
};

#define GetCallstack( name ) \
    CallstackData name; \
    name.count = backtrace( name.addr, 64 );

[[maybe_unused]] void PrintCallstack( const CallstackData& data, int skip = 0 );
void ShowExternalCallstacks( bool show );
