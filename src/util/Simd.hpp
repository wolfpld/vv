#pragma once

// Based on https://github.com/AcademySoftwareFoundation/OpenImageIO/blob/main/src/include/OpenImageIO/fmath.h

#if defined __SSE4_1__
#  include <x86intrin.h>
#endif

#if defined __SSE4_1__ && defined __FMA__

static inline __m128 _mm_log_ps( __m128 x )
{
    __m128i e0 = _mm_castps_si128( x );
    __m128i e1 = _mm_srai_epi32( e0, 23 );
    __m128i e2 = _mm_sub_epi32( e1, _mm_set1_epi32( 127 ) );
    __m128i e3 = _mm_and_si128( e0, _mm_set1_epi32( 0x007fffff ) );
    __m128i e4 = _mm_or_si128 ( e3, _mm_set1_epi32( 0x3f800000 ) );
    __m128 e5 = _mm_castsi128_ps( e4 );
    __m128 f = _mm_sub_ps( e5, _mm_set1_ps( 1.f ) );
    __m128 f2 = _mm_mul_ps( f, f );
    __m128 f4 = _mm_mul_ps( f2, f2 );
    __m128 hi = _mm_fmadd_ps( f, _mm_set1_ps( -0.00931049621349f ), _mm_set1_ps(  0.05206469089414f ) );
    __m128 lo = _mm_fmadd_ps( f, _mm_set1_ps(  0.47868480909345f ), _mm_set1_ps( -0.72116591947498f ) );
    hi = _mm_fmadd_ps( f, hi, _mm_set1_ps( -0.13753123777116f ) );
    hi = _mm_fmadd_ps( f, hi, _mm_set1_ps(  0.24187369696082f ) );
    hi = _mm_fmadd_ps( f, hi, _mm_set1_ps( -0.34730547155299f ) );
    lo = _mm_fmadd_ps( f, lo, _mm_set1_ps(  1.442689881667200f ) );
    __m128 r0 = _mm_mul_ps( f, lo );
    __m128 r1 = _mm_fmadd_ps( f4, hi, r0 );
    __m128 r2 = _mm_add_ps( r1, _mm_cvtepi32_ps( e2 ) );
    return r2;
}

static inline __m128 _mm_exp_ps( __m128 x )
{
    __m128i mi = _mm_cvtps_epi32( x );
    __m128  mf = _mm_round_ps( x, (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC) );
    x = _mm_sub_ps( x, mf );
    __m128 r = _mm_set1_ps( 1.33336498402e-3f );
    r = _mm_fmadd_ps( x, r, _mm_set1_ps( 9.810352697968e-3f ) );
    r = _mm_fmadd_ps( x, r, _mm_set1_ps( 5.551834031939e-2f ) );
    r = _mm_fmadd_ps( x, r, _mm_set1_ps( 0.2401793301105f ) );
    r = _mm_fmadd_ps( x, r, _mm_set1_ps( 0.693144857883f ) );
    r = _mm_fmadd_ps( x, r, _mm_set1_ps( 1.0f ) );
    __m128i m0 = _mm_slli_epi32( mi, 23 );
    __m128i ri = _mm_castps_si128( r );
    __m128i s = _mm_add_epi32( m0, ri );
    __m128 sf = _mm_castsi128_ps( s );
    return sf;
}

static inline __m128 _mm_pow_ps( __m128 x, __m128 y )
{
    return _mm_exp_ps( _mm_mul_ps( y, _mm_log_ps( x ) ) );
}

#endif

#ifdef __AVX2__
static inline __m256 _mm256_log_ps( __m256 x )
{
    __m256i e0 = _mm256_castps_si256( x );
    __m256i e1 = _mm256_srai_epi32( e0, 23 );
    __m256i e2 = _mm256_sub_epi32( e1, _mm256_set1_epi32( 127 ) );
    __m256i e3 = _mm256_and_si256( e0, _mm256_set1_epi32( 0x007fffff ) );
    __m256i e4 = _mm256_or_si256 ( e3, _mm256_set1_epi32( 0x3f800000 ) );
    __m256 e5 = _mm256_castsi256_ps( e4 );
    __m256 f = _mm256_sub_ps( e5, _mm256_set1_ps( 1.f ) );
    __m256 f2 = _mm256_mul_ps( f, f );
    __m256 f4 = _mm256_mul_ps( f2, f2 );
    __m256 hi = _mm256_fmadd_ps( f, _mm256_set1_ps( -0.00931049621349f ), _mm256_set1_ps(  0.05206469089414f ) );
    __m256 lo = _mm256_fmadd_ps( f, _mm256_set1_ps(  0.47868480909345f ), _mm256_set1_ps( -0.72116591947498f ) );
    hi = _mm256_fmadd_ps( f, hi, _mm256_set1_ps( -0.13753123777116f ) );
    hi = _mm256_fmadd_ps( f, hi, _mm256_set1_ps(  0.24187369696082f ) );
    hi = _mm256_fmadd_ps( f, hi, _mm256_set1_ps( -0.34730547155299f ) );
    lo = _mm256_fmadd_ps( f, lo, _mm256_set1_ps(  1.442689881667200f ) );
    __m256 r0 = _mm256_mul_ps( f, lo );
    __m256 r1 = _mm256_fmadd_ps( f4, hi, r0 );
    __m256 r2 = _mm256_add_ps( r1, _mm256_cvtepi32_ps( e2 ) );
    return r2;
}

static inline __m256 _mm256_exp_ps( __m256 x )
{
    __m256i mi = _mm256_cvtps_epi32( x );
    __m256  mf = _mm256_round_ps( x, (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC) );
    x = _mm256_sub_ps( x, mf );
    __m256 r = _mm256_set1_ps( 1.33336498402e-3f );
    r = _mm256_fmadd_ps( x, r, _mm256_set1_ps( 9.810352697968e-3f ) );
    r = _mm256_fmadd_ps( x, r, _mm256_set1_ps( 5.551834031939e-2f ) );
    r = _mm256_fmadd_ps( x, r, _mm256_set1_ps( 0.2401793301105f ) );
    r = _mm256_fmadd_ps( x, r, _mm256_set1_ps( 0.693144857883f ) );
    r = _mm256_fmadd_ps( x, r, _mm256_set1_ps( 1.0f ) );
    __m256i m0 = _mm256_slli_epi32( mi, 23 );
    __m256i ri = _mm256_castps_si256( r );
    __m256i s = _mm256_add_epi32( m0, ri );
    __m256 sf = _mm256_castsi256_ps( s );
    return sf;
}

static inline __m256 _mm256_pow_ps( __m256 x, __m256 y )
{
    return _mm256_exp_ps( _mm256_mul_ps( y, _mm256_log_ps( x ) ) );
}
#endif

#ifdef __AVX512F__
static inline __m512 _mm512_log_ps( __m512 x )
{
    __m512i e0 = _mm512_castps_si512( x );
    __m512i e1 = _mm512_srai_epi32( e0, 23 );
    __m512i e2 = _mm512_sub_epi32( e1, _mm512_set1_epi32( 127 ) );
    __m512i e3 = _mm512_and_epi32( e0, _mm512_set1_epi32( 0x007fffff ) );
    __m512i e4 = _mm512_or_epi32 ( e3, _mm512_set1_epi32( 0x3f800000 ) );
    __m512 e5 = _mm512_castsi512_ps( e4 );
    __m512 f = _mm512_sub_ps( e5, _mm512_set1_ps( 1.f ) );
    __m512 f2 = _mm512_mul_ps( f, f );
    __m512 f4 = _mm512_mul_ps( f2, f2 );
    __m512 hi = _mm512_fmadd_ps( f, _mm512_set1_ps( -0.00931049621349f ), _mm512_set1_ps(  0.05206469089414f ) );
    __m512 lo = _mm512_fmadd_ps( f, _mm512_set1_ps(  0.47868480909345f ), _mm512_set1_ps( -0.72116591947498f ) );
    hi = _mm512_fmadd_ps( f, hi, _mm512_set1_ps( -0.13753123777116f ) );
    hi = _mm512_fmadd_ps( f, hi, _mm512_set1_ps(  0.24187369696082f ) );
    hi = _mm512_fmadd_ps( f, hi, _mm512_set1_ps( -0.34730547155299f ) );
    lo = _mm512_fmadd_ps( f, lo, _mm512_set1_ps(  1.442689881667200f ) );
    __m512 r0 = _mm512_mul_ps( f, lo );
    __m512 r1 = _mm512_fmadd_ps( f4, hi, r0 );
    __m512 r2 = _mm512_add_ps( r1, _mm512_cvtepi32_ps( e2 ) );
    return r2;
}

static inline __m512 _mm512_exp_ps( __m512 x )
{
    __m512i mi = _mm512_cvtps_epi32( x );
    __m512  mf = _mm512_roundscale_ps( x, _MM_FROUND_TO_NEAREST_INT );
    x = _mm512_sub_ps( x, mf );
    __m512 r = _mm512_set1_ps( 1.33336498402e-3f );
    r = _mm512_fmadd_ps( x, r, _mm512_set1_ps( 9.810352697968e-3f ) );
    r = _mm512_fmadd_ps( x, r, _mm512_set1_ps( 5.551834031939e-2f ) );
    r = _mm512_fmadd_ps( x, r, _mm512_set1_ps( 0.2401793301105f ) );
    r = _mm512_fmadd_ps( x, r, _mm512_set1_ps( 0.693144857883f ) );
    r = _mm512_fmadd_ps( x, r, _mm512_set1_ps( 1.0f ) );
    __m512i m0 = _mm512_slli_epi32( mi, 23 );
    __m512i ri = _mm512_castps_si512( r );
    __m512i s = _mm512_add_epi32( m0, ri );
    __m512 sf = _mm512_castsi512_ps( s );
    return sf;
}

static inline __m512 _mm512_pow_ps( __m512 x, __m512 y )
{
    return _mm512_exp_ps( _mm512_mul_ps( y, _mm512_log_ps( x ) ) );
}
#endif
