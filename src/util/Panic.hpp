#pragma once

#include <stdlib.h>

#include "Logs.hpp"

#define CheckPanic( condition, msg, ... ) \
    { \
        if( !(condition) ) \
        { \
            mclog( LogLevel::Fatal, msg, ##__VA_ARGS__ ); \
            abort(); \
        } \
    }

#define Panic( msg, ... ) \
    { \
        mclog( LogLevel::Fatal, msg, ##__VA_ARGS__ ); \
        abort(); \
    }
