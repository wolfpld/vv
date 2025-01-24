#include <algorithm>
#include <cmath>
#include <libheif/heif.h>
#include <lcms2.h>
#include <string.h>

#include "HeifLoader.hpp"
#include "util/Alloca.h"
#include "util/Bitmap.hpp"
#include "util/BitmapHdr.hpp"
#include "util/FileBuffer.hpp"
#include "util/FileWrapper.hpp"
#include "util/Panic.hpp"
#include "util/Simd.hpp"
#include "util/TaskDispatch.hpp"
#include "util/Tonemapper.hpp"

namespace
{
constexpr cmsCIExyY white709 = { 0.3127f, 0.329f, 1 };
constexpr cmsCIExyYTRIPLE primaries709 = {
    { 0.64f, 0.33f, 1 },
    { 0.30f, 0.60f, 1 },
    { 0.15f, 0.06f, 1 }
};

float Pq( float N )
{
    constexpr float m1 = 0.1593017578125f;
    constexpr float m1inv = 1.f / m1;
    constexpr float m2 = 78.84375f;
    constexpr float m2inv = 1.f / m2;
    constexpr float c1 = 0.8359375f;
    constexpr float c2 = 18.8515625f;
    constexpr float c3 = 18.6875f;

    const auto Nm2 = std::pow( std::max( N, 0.f ), m2inv );
    return 10000.f * std::pow( std::max( 0.f, Nm2 - c1 ) / ( c2 - c3 * Nm2 ), m1inv ) / 255.f;
}

float Hlg( float E, float Y )
{
    constexpr float gamma = 1.2f;
    constexpr float g = ( gamma - 1.f ) / gamma;
    constexpr float alpha = 1.f;  // no idea about this value
    constexpr float invalpha = 1.f / alpha;

    return std::pow( Y * invalpha, g ) * E * invalpha;
}

#if defined __SSE4_1__ && defined __FMA__
void LinearizePq128( float* ptr, int sz )
{
    while( sz > 0 )
    {
        __m128 px0 = _mm_loadu_ps( ptr );
        __m128 px1 = _mm_max_ps( px0, _mm_setzero_ps() );
        __m128 Nm2 = _mm_pow_ps( px1, _mm_set1_ps( 1.f / 78.84375f ) );

        __m128 px2 = _mm_sub_ps( Nm2, _mm_set1_ps( 0.8359375f ) );
        __m128 px3 = _mm_max_ps( px2, _mm_setzero_ps() );

        __m128 px4 = _mm_fnmadd_ps( _mm_set1_ps( 18.6875f ), Nm2, _mm_set1_ps( 18.8515625f ) );
        __m128 px5 = _mm_div_ps( px3, px4 );

        __m128 px6 = _mm_pow_ps( px5, _mm_set1_ps( 1.f / 0.1593017578125f ) );
        __m128 ret = _mm_mul_ps( px6, _mm_set1_ps( 10000.f / 255.f ) );

        __m128 b = _mm_blend_ps( ret, px0, 0x8 );
        _mm_storeu_ps( ptr, b );

        ptr += 4;
        sz--;
    }
}

#  if defined __AVX2__
void LinearizePq256( float* ptr, int sz )
{
    while( sz > 1 )
    {
        __m256 px0 = _mm256_loadu_ps( ptr );
        __m256 px1 = _mm256_max_ps( px0, _mm256_setzero_ps() );
        __m256 Nm2 = _mm256_pow_ps( px1, _mm256_set1_ps( 1.f / 78.84375f ) );

        __m256 px2 = _mm256_sub_ps( Nm2, _mm256_set1_ps( 0.8359375f ) );
        __m256 px3 = _mm256_max_ps( px2, _mm256_setzero_ps() );

        __m256 px4 = _mm256_fnmadd_ps( _mm256_set1_ps( 18.6875f ), Nm2, _mm256_set1_ps( 18.8515625f ) );
        __m256 px5 = _mm256_div_ps( px3, px4 );

        __m256 px6 = _mm256_pow_ps( px5, _mm256_set1_ps( 1.f / 0.1593017578125f ) );
        __m256 ret = _mm256_mul_ps( px6, _mm256_set1_ps( 10000.f / 255.f ) );

        __m256 b = _mm256_blend_ps( ret, px0, 0x88 );
        _mm256_storeu_ps( ptr, b );

        ptr += 8;
        sz -= 2;
    }
}
#  endif

#  if defined __AVX512F__
void LinearizePq512( float* ptr, int sz )
{
    while( sz > 3 )
    {
        __m512 px0 = _mm512_loadu_ps( ptr );
        __m512 px1 = _mm512_max_ps( px0, _mm512_setzero_ps() );
        __m512 Nm2 = _mm512_pow_ps( px1, _mm512_set1_ps( 1.f / 78.84375f ) );

        __m512 px2 = _mm512_sub_ps( Nm2, _mm512_set1_ps( 0.8359375f ) );
        __m512 px3 = _mm512_max_ps( px2, _mm512_setzero_ps() );

        __m512 px4 = _mm512_fnmadd_ps( _mm512_set1_ps( 18.6875f ), Nm2, _mm512_set1_ps( 18.8515625f ) );
        __m512 px5 = _mm512_div_ps( px3, px4 );

        __m512 px6 = _mm512_pow_ps( px5, _mm512_set1_ps( 1.f / 0.1593017578125f ) );
        __m512 ret = _mm512_mul_ps( px6, _mm512_set1_ps( 10000.f / 255.f ) );

        __m512 b = _mm512_mask_blend_ps( 0x8888, ret, px0 );
        _mm512_storeu_ps( ptr, b );

        ptr += 16;
        sz -= 4;
    }
}
#  endif

void LinearizePq( float* ptr, int sz )
{
#  ifdef __AVX512F__
    LinearizePq512( ptr, sz );
    if( (sz & 3) == 0 ) return;
    ptr += ( sz & ~3 ) * 4;
    sz &= 3;
#  endif
#  ifdef __AVX2__
    LinearizePq256( ptr, sz );
    if( (sz & 1) == 0 ) return;
    ptr += ( sz & ~1 ) * 4;
    sz &= 1;
#  endif
    LinearizePq128( ptr, sz );
}
#else
void LinearizePq( float* ptr, int sz )
{
    for( int i=0; i<sz; i++ )
    {
        ptr[0] = Pq( ptr[0] );
        ptr[1] = Pq( ptr[1] );
        ptr[2] = Pq( ptr[2] );

        ptr += 4;
    }
}
#endif
}

HeifLoader::HeifLoader( std::shared_ptr<FileWrapper> file, ToneMap::Operator tonemap, TaskDispatch* td )
    : ImageLoader( std::move( file ) )
    , m_valid( false )
    , m_tonemap( tonemap )
    , m_ctx( nullptr )
    , m_handle( nullptr )
    , m_image( nullptr )
    , m_nclx( nullptr )
    , m_iccData( nullptr )
    , m_profileIn( nullptr )
    , m_profileOut( nullptr )
    , m_transform( nullptr )
    , m_td( td )
{
    fseek( *m_file, 0, SEEK_SET );
    uint8_t hdr[12];
    if( fread( hdr, 1, 12, *m_file ) == 12 )
    {
        const auto res = heif_check_filetype( hdr, 12 );
        m_valid = res == heif_filetype_yes_supported || res == heif_filetype_maybe;
    }
}

HeifLoader::~HeifLoader()
{
    if( m_transform ) cmsDeleteTransform( m_transform );
    if( m_profileOut ) cmsCloseProfile( m_profileOut );
    if( m_profileIn ) cmsCloseProfile( m_profileIn );
    if( m_iccData ) delete[] m_iccData;
    if( m_image ) heif_image_release( m_image );
    if( m_handle ) heif_image_handle_release( m_handle );
    if( m_ctx ) heif_context_free( m_ctx );
}

bool HeifLoader::IsValid() const
{
    return m_valid;
}

bool HeifLoader::IsHdr()
{
    if( !m_buf && !Open() ) return false;
    if( m_nclx )
    {
        return
            m_nclx->transfer_characteristics == heif_transfer_characteristic_ITU_R_BT_2100_0_PQ ||
            m_nclx->transfer_characteristics == heif_transfer_characteristic_ITU_R_BT_2100_0_HLG;
    }
    return false;
}

std::unique_ptr<Bitmap> HeifLoader::Load()
{
    if( !m_buf && !Open() ) return nullptr;

    if( !IsHdr() )
    {
        if( !m_iccData )
        {
            return LoadNoProfile();
        }
        else
        {
            if( !SetupDecode( false ) ) return nullptr;

            auto bmp = std::make_unique<Bitmap>( m_width, m_height );
            auto out = (uint32_t*)bmp->Data();

            if( m_td )
            {
                size_t offset = 0;
                size_t sz = m_width * m_height;
                while( sz > 0 )
                {
                    const auto chunk = std::min( sz, size_t( 16 * 1024 ) );
                    m_td->Queue( [this, out, chunk, offset] {
                        auto ptr = (float*)alloca( chunk * 4 * sizeof( float ) );
                        LoadYCbCr( ptr, chunk, offset );
                        ConvertYCbCrToRGB( ptr, chunk );
                        cmsDoTransform( m_transform, ptr, out, chunk );
                    } );
                    out += chunk;
                    sz -= chunk;
                    offset += chunk;
                }
                m_td->Sync();
            }
            else
            {
                auto tmp = std::make_unique<BitmapHdr>( m_width, m_height );
                LoadYCbCr( tmp->Data(), m_width * m_height, 0 );
                ConvertYCbCrToRGB( tmp->Data(), m_width * m_height );
                cmsDoTransform( m_transform, tmp->Data(), out, m_width * m_height );
            }

            return bmp;
        }
    }
    else
    {
        if( m_td )
        {
            if( !SetupDecode( true ) ) return nullptr;

            auto bmp = std::make_unique<Bitmap>( m_width, m_height );
            auto out = (uint32_t*)bmp->Data();

            size_t offset = 0;
            size_t sz = m_width * m_height;
            while( sz > 0 )
            {
                const auto chunk = std::min( sz, size_t( 16 * 1024 ) );
                m_td->Queue( [this, out, chunk, offset] {
                    auto ptr = (float*)alloca( chunk * 4 * sizeof( float ) );
                    LoadYCbCr( ptr, chunk, offset );
                    ConvertYCbCrToRGB( ptr, chunk );
                    if( m_transform ) cmsDoTransform( m_transform, ptr, ptr, chunk );
                    ApplyTransfer( ptr, chunk );
                    ToneMap::Process( m_tonemap, out, ptr, chunk );
                } );
                out += chunk;
                sz -= chunk;
                offset += chunk;
            }
            m_td->Sync();
            return bmp;
        }
        else
        {
            std::unique_ptr<BitmapHdr> hdr = LoadHdr();
            return hdr->Tonemap( m_tonemap );
        }
    }
}

std::unique_ptr<BitmapHdr> HeifLoader::LoadHdr()
{
    if( !m_buf && !Open() ) return nullptr;
    if( !SetupDecode( true ) ) return nullptr;

    auto bmp = std::make_unique<BitmapHdr>( m_width, m_height );
    if( m_td )
    {
        auto ptr = bmp->Data();
        size_t offset = 0;
        size_t sz = m_width * m_height;
        while( sz > 0 )
        {
            const auto chunk = std::min( sz, size_t( 16 * 1024 ) );
            m_td->Queue( [this, ptr, chunk, offset] {
                LoadYCbCr( ptr, chunk, offset );
                ConvertYCbCrToRGB( ptr, chunk );
                if( m_transform ) cmsDoTransform( m_transform, ptr, ptr, chunk );
                ApplyTransfer( ptr, chunk );
            } );
            ptr += chunk * 4;
            sz -= chunk;
            offset += chunk;
        }
        m_td->Sync();
    }
    else
    {
        LoadYCbCr( bmp->Data(), m_width * m_height, 0 );
        ConvertYCbCrToRGB( bmp->Data(), m_width * m_height );
        if( m_transform ) cmsDoTransform( m_transform, bmp->Data(), bmp->Data(), m_width * m_height );
        ApplyTransfer( bmp->Data(), m_width * m_height );
    }

    return bmp;
}

bool HeifLoader::Open()
{
    CheckPanic( m_valid, "Invalid HEIF file" );
    CheckPanic( !m_buf, "Already opened" );

    m_buf = std::make_unique<FileBuffer>( m_file );

    m_ctx = heif_context_alloc();
    auto err = heif_context_read_from_memory_without_copy( m_ctx, m_buf->data(), m_buf->size(), nullptr );
    if( err.code != heif_error_Ok ) return false;

    err = heif_context_get_primary_image_handle( m_ctx, &m_handle );
    if( err.code != heif_error_Ok ) return false;

    err = heif_image_handle_get_nclx_color_profile( m_handle, &m_nclx );
    if( err.code == heif_error_Color_profile_does_not_exist ) mclog( LogLevel::Info, "HEIF: No handle-level nclx color profile found" );

    m_iccSize = heif_image_handle_get_raw_color_profile_size( m_handle );
    if( m_iccSize == 0 )
    {
        mclog( LogLevel::Info, "HEIF: No ICC profile found" );
    }
    else
    {
        m_iccData = new char[m_iccSize];
        err = heif_image_handle_get_raw_color_profile( m_handle, m_iccData );
        CheckPanic( err.code == heif_error_Ok, "HEIF: Failed to read ICC profile" );
    }

    m_width = heif_image_handle_get_width( m_handle );
    m_height = heif_image_handle_get_height( m_handle );

    return true;
}

bool HeifLoader::SetupDecode( bool hdr )
{
    auto err = heif_decode_image( m_handle, &m_image, heif_colorspace_YCbCr, heif_chroma_444, nullptr );
    if( err.code != heif_error_Ok ) return false;

    if( !m_nclx )
    {
        auto err = heif_image_get_nclx_color_profile( m_image, &m_nclx );
        if( err.code == heif_error_Color_profile_does_not_exist ) mclog( LogLevel::Info, "HEIF: No image-level nclx color profile found" );
    }

    int strideY, strideCb, strideCr, strideA;
    m_planeY  = heif_image_get_plane_readonly( m_image, heif_channel_Y, &strideY );
    m_planeCb = heif_image_get_plane_readonly( m_image, heif_channel_Cb, &strideCb );
    m_planeCr = heif_image_get_plane_readonly( m_image, heif_channel_Cr, &strideCr );
    m_planeA  = heif_image_get_plane_readonly( m_image, heif_channel_Alpha, &strideA );
    CheckPanic( strideY == strideCb && strideY == strideCr, "Decoded output is not 444" );
    m_stride = strideY;

    if( !m_planeY || !m_planeCb || !m_planeCr ) return false;

    const auto bppY = heif_image_get_bits_per_pixel_range( m_image, heif_channel_Y );
    const auto bppCb = heif_image_get_bits_per_pixel_range( m_image, heif_channel_Cb );
    const auto bppCr = heif_image_get_bits_per_pixel_range( m_image, heif_channel_Cr );
    CheckPanic( bppY == bppCb && bppY == bppCr, "Decoded output has mixed bit width" );

    m_bpp = bppY;
    m_bppDiv = 1.f / ( ( 1 << bppY ) - 1 );
    mclog( LogLevel::Info, "HEIF: %d bpp", bppY );

    if( bppY > 8 ) m_stride /= 2;

    // H.273, 8.3, VideoFullRangeFlag is false if not present
    if( !m_nclx || !m_nclx->full_range_flag )
    {
        mclog( LogLevel::Info, "HEIF: Full range flag not set, converting to full range" );
    }

    m_matrix = Conversion::BT601;
    if( m_nclx )
    {
        switch( m_nclx->matrix_coefficients )
        {
        case heif_matrix_coefficients_RGB_GBR:
            m_matrix = Conversion::GBR;
            mclog( LogLevel::Info, "HEIF: Matrix coefficients GBR" );
            break;
        case heif_matrix_coefficients_ITU_R_BT_709_5:
            m_matrix = Conversion::BT709;
            mclog( LogLevel::Info, "HEIF: Matrix coefficients BT.709" );
            break;
        case heif_matrix_coefficients_unspecified:      // see https://github.com/AOMediaCodec/libavif/wiki/CICP
        case heif_matrix_coefficients_ITU_R_BT_470_6_System_B_G:
        case heif_matrix_coefficients_ITU_R_BT_601_6:
            m_matrix = Conversion::BT601;
            mclog( LogLevel::Info, "HEIF: Matrix coefficients BT.601" );
            break;
        case heif_matrix_coefficients_ITU_R_BT_2020_2_non_constant_luminance:
        case heif_matrix_coefficients_ITU_R_BT_2020_2_constant_luminance:
            m_matrix = Conversion::BT2020;
            mclog( LogLevel::Info, "HEIF: Matrix coefficients BT.2020" );
            break;
        default:
            mclog( LogLevel::Error, "HEIF: Matrix coefficients %d not implemented, defaulting to BT.601", m_nclx->matrix_coefficients );
            break;
        }

        switch( m_nclx->transfer_characteristics )
        {
        case heif_transfer_characteristic_unspecified:
        case heif_transfer_characteristic_IEC_61966_2_1:
            mclog( LogLevel::Info, "HEIF: Ignoring IEC 61966-2-1 transfer function" );
            break;
        case heif_transfer_characteristic_ITU_R_BT_2100_0_PQ:
            mclog( LogLevel::Info, "HEIF: Applying PQ transfer function" );
            break;
        case heif_transfer_characteristic_ITU_R_BT_2100_0_HLG:
            mclog( LogLevel::Info, "HEIF: Applying HLG transfer function" );
            break;
        default:
            mclog( LogLevel::Error, "HEIF: Transfer characteristics %d not implemented", m_nclx->transfer_characteristics );
            break;
        }
    }

    if( m_iccData )
    {
        m_profileIn = cmsOpenProfileFromMem( m_iccData, m_iccSize );
        int outType;
        if( hdr )
        {
            cmsToneCurve* linear = cmsBuildGamma( nullptr, 1 );
            cmsToneCurve* linear3[3] = { linear, linear, linear };
            m_profileOut = cmsCreateRGBProfile( &white709, &primaries709, linear3 );
            outType = TYPE_RGBA_FLT;
            cmsFreeToneCurve( linear );
        }
        else
        {
            m_profileOut = cmsCreate_sRGBProfile();
            outType = TYPE_RGBA_8;
        }
        m_transform = cmsCreateTransform( m_profileIn, TYPE_RGBA_FLT, m_profileOut, outType, INTENT_PERCEPTUAL, cmsFLAGS_COPY_ALPHA );
    }
    else
    {
        CheckPanic( m_nclx, "No color profile code path should be taken." );
        CheckPanic( hdr, "No color profile code path should be taken." );

        if( m_nclx->color_primaries != heif_color_primaries_ITU_R_BT_709_5 )
        {
            cmsToneCurve* linear = cmsBuildGamma( nullptr, 1 );
            cmsToneCurve* linear3[3] = { linear, linear, linear };

            const cmsCIExyY white = { m_nclx->color_primary_white_x, m_nclx->color_primary_white_y, 1 };
            const cmsCIExyYTRIPLE primaries = {
                { m_nclx->color_primary_red_x, m_nclx->color_primary_red_y, 1 },
                { m_nclx->color_primary_green_x, m_nclx->color_primary_green_y, 1 },
                { m_nclx->color_primary_blue_x, m_nclx->color_primary_blue_y, 1 }
            };

            m_profileIn = cmsCreateRGBProfile( &white, &primaries, linear3 );
            m_profileOut = cmsCreateRGBProfile( &white709, &primaries709, linear3 );
            cmsFreeToneCurve( linear );
            m_transform = cmsCreateTransform( m_profileIn, TYPE_RGBA_FLT, m_profileOut, TYPE_RGBA_FLT, INTENT_PERCEPTUAL, cmsFLAGS_COPY_ALPHA );
        }
    }

    return true;
}

std::unique_ptr<Bitmap> HeifLoader::LoadNoProfile()
{
    heif_image* img;
    auto err = heif_decode_image( m_handle, &img, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, nullptr );
    if( err.code != heif_error_Ok ) return nullptr;

    int stride;
    auto src = heif_image_get_plane_readonly( img, heif_channel_interleaved, &stride );
    if( !src )
    {
        heif_image_release( img );
        return nullptr;
    }

    auto bmp = std::make_unique<Bitmap>( m_width, m_height );
    if( stride == m_width * 4 )
    {
        memcpy( bmp->Data(), src, m_width * m_height * 4 );
    }
    else
    {
        auto dst = bmp->Data();
        for( int i=0; i<m_height; i++ )
        {
            memcpy( dst, src, m_width * 4 );
            dst += m_width * 4;
            src += stride;
        }
    }

    heif_image_release( img );
    return bmp;
}

template<typename T>
static inline void ProcessYCbCrAlpha( float* ptr, const T* srcY, const T* srcCb, const T* srcCr, const T* srcA, size_t sz, size_t offset, size_t width, size_t stride, float div )
{
    const auto gap = stride - width;

    const int py = offset / width;
    int px = offset % width;

    srcY += py * stride + px;
    srcCb += py * stride + px;
    srcCr += py * stride + px;
    srcA += py * stride + px;

    for(;;)
    {
        auto line = std::min( sz, size_t( width - px ) );
        sz -= line;
        while( line-- )
        {
            *ptr++ = float(*srcY++) * div;
            *ptr++ = float(*srcCb++) * div - 0.5f;
            *ptr++ = float(*srcCr++) * div - 0.5f;
            *ptr++ = float(*srcA++) * div;
        }
        if( sz == 0 ) break;
        px = 0;

        srcY += gap;
        srcCb += gap;
        srcCr += gap;
        srcA += gap;
    }
}

template<typename T>
static inline void ProcessYCbCr( float* ptr, const T* srcY, const T* srcCb, const T* srcCr, size_t sz, size_t offset, size_t width, size_t stride, float div )
{
    const auto gap = stride - width;

    const int py = offset / width;
    int px = offset % width;

    srcY += py * stride + px;
    srcCb += py * stride + px;
    srcCr += py * stride + px;

    for(;;)
    {
        auto line = std::min( sz, size_t( width - px ) );
        sz -= line;
        while( line-- )
        {
            *ptr++ = float(*srcY++) * div;
            *ptr++ = float(*srcCb++) * div - 0.5f;
            *ptr++ = float(*srcCr++) * div - 0.5f;
            *ptr++ = 1.f;
        }
        if( sz == 0 ) break;
        px = 0;

        srcY += gap;
        srcCb += gap;
        srcCr += gap;
    }
}

void HeifLoader::LoadYCbCr( float* ptr, size_t sz, size_t offset )
{
    if( m_planeA )
    {
        if( m_bpp > 8 )
        {
            ProcessYCbCrAlpha( ptr, (uint16_t*)m_planeY, (uint16_t*)m_planeCb, (uint16_t*)m_planeCr, (uint16_t*)m_planeA, sz, offset, m_width, m_stride, m_bppDiv );
        }
        else
        {
            ProcessYCbCrAlpha( ptr, m_planeY, m_planeCb, m_planeCr, m_planeA, sz, offset, m_width, m_stride, m_bppDiv );
        }
    }
    else
    {
        if( m_bpp > 8 )
        {
            ProcessYCbCr( ptr, (uint16_t*)m_planeY, (uint16_t*)m_planeCb, (uint16_t*)m_planeCr, sz, offset, m_width, m_stride, m_bppDiv );
        }
        else
        {
            ProcessYCbCr( ptr, m_planeY, m_planeCb, m_planeCr, sz, offset, m_width, m_stride, m_bppDiv );
        }
    }

    // H.273, 8.3, VideoFullRangeFlag is false if not present
    if( !m_nclx || !m_nclx->full_range_flag )
    {
        constexpr float scale = 255.f / 219.f;
        constexpr float offset = 16.f / 255.f;

        do
        {
            ptr[0] = ( ptr[0] - offset ) * scale;
            ptr += 4;
        }
        while( --sz );
    }
}

void HeifLoader::ConvertYCbCrToRGB( float* ptr, size_t sz )
{
    if( m_matrix == Conversion::GBR )
    {
        do
        {
            const auto g = ptr[0];
            const auto b = ptr[1] + 0.5f;
            const auto r = ptr[2] + 0.5f;

            ptr[0] = r;
            ptr[1] = g;
            ptr[2] = b;

            ptr += 4;
        }
        while( --sz );
    }
    else
    {
        float a, b, c, d;
        switch( m_matrix )
        {
        case Conversion::BT601:
            a = 1.402f;
            b = -0.344136f;
            c = -0.714136f;
            d = 1.772f;
            break;
        case Conversion::BT709:
            a = 1.5748f;
            b = -0.1873f;
            c = -0.4681f;
            d = 1.8556f;
            break;
        case Conversion::BT2020:
            a = 1.4746f;
            b = -0.16455312684366f;
            c = -0.57135312684366f;
            d = 1.8814f;
            break;
        default:
            Panic( "Invalid conversion matrix" );
            break;
        }

        do
        {
            const auto Y = ptr[0];
            const auto Cb = ptr[1];
            const auto Cr = ptr[2];

            const auto r = Y + a * Cr;
            const auto g = Y + b * Cb + c * Cr;
            const auto b = Y + d * Cb;

            ptr[0] = r;
            ptr[1] = g;
            ptr[2] = b;

            ptr += 4;
        }
        while( --sz );
    }
}

void HeifLoader::ApplyTransfer( float* ptr, size_t sz )
{
    CheckPanic( m_nclx, "No nclx color profile found" );

    switch( m_nclx->transfer_characteristics )
    {
    case heif_transfer_characteristic_ITU_R_BT_2100_0_PQ:
        LinearizePq( ptr, sz );
        break;
    case heif_transfer_characteristic_ITU_R_BT_2100_0_HLG:
        do
        {
            const auto r = ptr[0];
            const auto g = ptr[1];
            const auto b = ptr[2];

            const auto Y = 0.2627f * r + 0.6780f * g + 0.0593f * b;

            ptr[0] = Hlg( r, Y );
            ptr[1] = Hlg( g, Y );
            ptr[2] = Hlg( b, Y );

            ptr += 4;
        }
        while( --sz );
        break;
    default:
        break;
    }
}
