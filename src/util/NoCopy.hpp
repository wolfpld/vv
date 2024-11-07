#pragma once

#define NoCopy( T ) \
    T( const T& ) = delete; \
    T& operator=( const T& ) = delete
