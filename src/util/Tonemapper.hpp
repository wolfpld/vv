#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ToneMap
{

enum class Operator
{
    AgX,
    AgXGolden,
    AgXPunchy,
    PbrNeutral
};

void Process( Operator op, uint32_t* dst, float* src, size_t sz );


struct HdrColor
{
    float r;
    float g;
    float b;
    float a;
};

float LinearToSrgb( float x );

void AgX( uint32_t* dst, float* src, size_t sz );
void AgXGolden( uint32_t* dst, float* src, size_t sz );
void AgXPunchy( uint32_t* dst, float* src, size_t sz );
void PbrNeutral( uint32_t* dst, float* src, size_t sz );

}
