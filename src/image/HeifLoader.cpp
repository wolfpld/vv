#include <algorithm>
#include <cmath>
#include <libheif/heif.h>
#include <lcms2.h>
#include <string.h>

#include "HeifLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/BitmapHdr.hpp"
#include "util/FileBuffer.hpp"
#include "util/FileWrapper.hpp"
#include "util/Panic.hpp"

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
}

HeifLoader::HeifLoader( std::shared_ptr<FileWrapper> file )
    : ImageLoader( std::move( file ) )
    , m_valid( false )
    , m_ctx( nullptr )
    , m_handle( nullptr )
    , m_nclx( nullptr )
    , m_iccData( nullptr )
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
    if( m_iccData ) delete[] m_iccData;
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

    if( !m_nclx )
    {
        if( !m_iccData )
        {
            return LoadNoProfile();
        }
        else
        {
            return LoadWithIccProfile();
        }
    }
    else
    {
        std::unique_ptr<BitmapHdr> hdr = LoadHdr();
        return hdr->Tonemap();
    }
}

std::unique_ptr<BitmapHdr> HeifLoader::LoadHdr()
{
    if( !m_buf && !Open() ) return nullptr;

    if( !m_iccData )
    {
        return LoadHdrNoProfile();
    }
    else
    {
        return LoadHdrWithIccProfile();
    }
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

std::unique_ptr<Bitmap> HeifLoader::LoadWithIccProfile()
{
    auto tmp = LoadYCbCr();
    if( !tmp ) return nullptr;
    ConvertYCbCrToRGB( tmp );

    auto profileIn = cmsOpenProfileFromMem( m_iccData, m_iccSize );
    auto profileOut = cmsCreate_sRGBProfile();
    auto transform = cmsCreateTransform( profileIn, TYPE_RGBA_FLT, profileOut, TYPE_RGBA_8, INTENT_PERCEPTUAL, cmsFLAGS_COPY_ALPHA );

    auto bmp = std::make_unique<Bitmap>( m_width, m_height );
    cmsDoTransform( transform, tmp->Data(), bmp->Data(), m_width * m_height );

    cmsDeleteTransform( transform );
    cmsCloseProfile( profileOut );
    cmsCloseProfile( profileIn );

    return bmp;
}

std::unique_ptr<BitmapHdr> HeifLoader::LoadHdrNoProfile()
{
    CheckPanic( m_nclx, "No nclx color profile found" );

    auto bmp = LoadYCbCr();
    if( !bmp ) return nullptr;
    ConvertYCbCrToRGB( bmp );

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

        auto profileIn = cmsCreateRGBProfile( &white, &primaries, linear3 );
        auto profileOut = cmsCreateRGBProfile( &white709, &primaries709, linear3 );
        auto transform = cmsCreateTransform( profileIn, TYPE_RGBA_FLT, profileOut, TYPE_RGBA_FLT, INTENT_PERCEPTUAL, cmsFLAGS_COPY_ALPHA );

        auto corrected = std::make_unique<BitmapHdr>( m_width, m_height );
        cmsDoTransform( transform, bmp->Data(), corrected->Data(), m_width * m_height );
        std::swap( bmp, corrected );

        cmsDeleteTransform( transform );
        cmsCloseProfile( profileOut );
        cmsCloseProfile( profileIn );
        cmsFreeToneCurve( linear );
    }

    ApplyTransfer( bmp );
    return bmp;
}

std::unique_ptr<BitmapHdr> HeifLoader::LoadHdrWithIccProfile()
{
    CheckPanic( m_nclx, "No nclx color profile found" );

    auto bmp = LoadYCbCr();
    if( !bmp ) return nullptr;
    ConvertYCbCrToRGB( bmp );

    cmsToneCurve* linear = cmsBuildGamma( nullptr, 1 );
    cmsToneCurve* linear3[3] = { linear, linear, linear };

    auto profileIn = cmsOpenProfileFromMem( m_iccData, m_iccSize );
    auto profileOut = cmsCreateRGBProfile( &white709, &primaries709, linear3 );
    auto transform = cmsCreateTransform( profileIn, TYPE_RGBA_FLT, profileOut, TYPE_RGBA_FLT, INTENT_PERCEPTUAL, cmsFLAGS_COPY_ALPHA );

    auto corrected = std::make_unique<BitmapHdr>( m_width, m_height );
    cmsDoTransform( transform, bmp->Data(), corrected->Data(), m_width * m_height );

    cmsDeleteTransform( transform );
    cmsCloseProfile( profileOut );
    cmsCloseProfile( profileIn );
    cmsFreeToneCurve( linear );

    ApplyTransfer( bmp );
    return corrected;
}

std::unique_ptr<BitmapHdr> HeifLoader::LoadYCbCr()
{
    heif_image* img;
    auto err = heif_decode_image( m_handle, &img, heif_colorspace_YCbCr, heif_chroma_444, nullptr );
    if( err.code != heif_error_Ok ) return nullptr;

    if( !m_nclx )
    {
        auto err = heif_image_get_nclx_color_profile( img, &m_nclx );
        if( err.code == heif_error_Color_profile_does_not_exist ) mclog( LogLevel::Info, "HEIF: No image-level nclx color profile found" );
    }

    int strideY, strideCb, strideCr, strideA;
    auto srcY = heif_image_get_plane_readonly( img, heif_channel_Y, &strideY );
    auto srcCb = heif_image_get_plane_readonly( img, heif_channel_Cb, &strideCb );
    auto srcCr = heif_image_get_plane_readonly( img, heif_channel_Cr, &strideCr );
    auto srcA = heif_image_get_plane_readonly( img, heif_channel_Alpha, &strideA );
    CheckPanic( strideY == strideCb && strideY == strideCr, "Decoded output is not 444" );

    if( !srcY || !srcCb || !srcCr )
    {
        heif_image_release( img );
        return nullptr;
    }

    const auto bppY = heif_image_get_bits_per_pixel_range( img, heif_channel_Y );
    const auto bppCb = heif_image_get_bits_per_pixel_range( img, heif_channel_Cb );
    const auto bppCr = heif_image_get_bits_per_pixel_range( img, heif_channel_Cr );
    CheckPanic( bppY == bppCb && bppY == bppCr, "Decoded output has mixed bit width" );

    const float div = 1.f / ( ( 1 << bppY ) - 1 );
    mclog( LogLevel::Info, "HEIF: %d bpp", bppY );

    auto bmp = std::make_unique<BitmapHdr>( m_width, m_height );
    auto dst = bmp->Data();
    if( srcA )
    {
        if( bppY > 8 )
        {
            auto srcY16 = (uint16_t*)srcY;
            auto srcCb16 = (uint16_t*)srcCb;
            auto srcCr16 = (uint16_t*)srcCr;
            auto srcA16 = (uint16_t*)srcA;

            strideY /= 2;
            strideCb /= 2;
            strideCr /= 2;
            strideA /= 2;

            for( int i=0; i<m_height; i++ )
            {
                for( int j=0; j<m_width; j++ )
                {
                    const auto Y = float(*srcY16++) * div;
                    const auto Cb = float(*srcCb16++) * div - 0.5f;
                    const auto Cr = float(*srcCr16++) * div - 0.5f;
                    const auto a = float(*srcA16++) * div;

                    *dst++ = Y;
                    *dst++ = Cb;
                    *dst++ = Cr;
                    *dst++ = a;
                }

                srcY16 += strideY - m_width;
                srcCb16 += strideCb - m_width;
                srcCr16 += strideCr - m_width;
                srcA16 += strideA - m_width;
            }
        }
        else
        {
            for( int i=0; i<m_height; i++ )
            {
                for( int j=0; j<m_width; j++ )
                {
                    const auto Y = float(*srcY++) * div;
                    const auto Cb = float(*srcCb++) * div - 0.5f;
                    const auto Cr = float(*srcCr++) * div - 0.5f;
                    const auto a = float(*srcA++) * div;

                    *dst++ = Y;
                    *dst++ = Cb;
                    *dst++ = Cr;
                    *dst++ = a;
                }

                srcY += strideY - m_width;
                srcCb += strideCb - m_width;
                srcCr += strideCr - m_width;
                srcA += strideA - m_width;
            }
        }
    }
    else
    {
        if( bppY > 8 )
        {
            auto srcY16 = (uint16_t*)srcY;
            auto srcCb16 = (uint16_t*)srcCb;
            auto srcCr16 = (uint16_t*)srcCr;

            strideY /= 2;
            strideCb /= 2;
            strideCr /= 2;

            for( int i=0; i<m_height; i++ )
            {
                for( int j=0; j<m_width; j++ )
                {
                    const auto Y = float(*srcY16++) * div;
                    const auto Cb = float(*srcCb16++) * div - 0.5f;
                    const auto Cr = float(*srcCr16++) * div - 0.5f;

                    *dst++ = Y;
                    *dst++ = Cb;
                    *dst++ = Cr;
                    *dst++ = 1.f;
                }

                srcY16 += strideY - m_width;
                srcCb16 += strideCb - m_width;
                srcCr16 += strideCr - m_width;
            }
        }
        else
        {
            for( int i=0; i<m_height; i++ )
            {
                for( int j=0; j<m_width; j++ )
                {
                    const auto Y = float(*srcY++) * div;
                    const auto Cb = float(*srcCb++) * div - 0.5f;
                    const auto Cr = float(*srcCr++) * div - 0.5f;

                    *dst++ = Y;
                    *dst++ = Cb;
                    *dst++ = Cr;
                    *dst++ = 1.f;
                }

                srcY += strideY - m_width;
                srcCb += strideCb - m_width;
                srcCr += strideCr - m_width;
            }
        }
    }

    // H.273, 8.3, VideoFullRangeFlag is false if not present
    if( !m_nclx || !m_nclx->full_range_flag )
    {
        mclog( LogLevel::Info, "HEIF: Full range flag not set, converting to full range" );

        constexpr float scale = 255.f / 219.f;
        constexpr float offset = 16.f / 255.f;

        auto ptr = bmp->Data();
        auto sz = bmp->Width() * bmp->Height();
        do
        {
            ptr[0] = ( ptr[0] - offset ) * scale;
            ptr += 4;
        }
        while( --sz );
    }

    return bmp;
}

void HeifLoader::ConvertYCbCrToRGB( const std::unique_ptr<BitmapHdr>& bmp )
{
    enum class Conversion
    {
        GBR,
        BT601,
        BT709,
        BT2020
    };

    auto cs = Conversion::BT601;
    if( m_nclx )
    {
        switch( m_nclx->matrix_coefficients )
        {
        case heif_matrix_coefficients_RGB_GBR:
            cs = Conversion::GBR;
            mclog( LogLevel::Info, "HEIF: Matrix coefficients GBR" );
            break;
        case heif_matrix_coefficients_ITU_R_BT_709_5:
            cs = Conversion::BT709;
            mclog( LogLevel::Info, "HEIF: Matrix coefficients BT.709" );
            break;
        case heif_matrix_coefficients_unspecified:      // see https://github.com/AOMediaCodec/libavif/wiki/CICP
        case heif_matrix_coefficients_ITU_R_BT_470_6_System_B_G:
        case heif_matrix_coefficients_ITU_R_BT_601_6:
            cs = Conversion::BT601;
            mclog( LogLevel::Info, "HEIF: Matrix coefficients BT.601" );
            break;
        case heif_matrix_coefficients_ITU_R_BT_2020_2_non_constant_luminance:
        case heif_matrix_coefficients_ITU_R_BT_2020_2_constant_luminance:
            cs = Conversion::BT2020;
            mclog( LogLevel::Info, "HEIF: Matrix coefficients BT.2020" );
            break;
        default:
            mclog( LogLevel::Error, "HEIF: Matrix coefficients %d not implemented, defaulting to BT.601", m_nclx->matrix_coefficients );
            break;
        }
    }

    if( cs == Conversion::GBR )
    {
        auto ptr = bmp->Data();
        auto sz = bmp->Width() * bmp->Height();
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
        switch( cs )
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

        auto ptr = bmp->Data();
        auto sz = bmp->Width() * bmp->Height();
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

void HeifLoader::ApplyTransfer( const std::unique_ptr<BitmapHdr>& bmp )
{
    CheckPanic( m_nclx, "No nclx color profile found" );

    auto ptr = bmp->Data();
    auto sz = bmp->Width() * bmp->Height();

    switch( m_nclx->transfer_characteristics )
    {
    case heif_transfer_characteristic_ITU_R_BT_2100_0_PQ:
        mclog( LogLevel::Info, "HEIF: Applying PQ transfer function" );
        do
        {
            ptr[0] = Pq( ptr[0] );
            ptr[1] = Pq( ptr[1] );
            ptr[2] = Pq( ptr[2] );

            ptr += 4;
        }
        while( --sz );
        break;
    case heif_transfer_characteristic_ITU_R_BT_2100_0_HLG:
        mclog( LogLevel::Info, "HEIF: Applying HLG transfer function" );
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
        mclog( LogLevel::Error, "HEIF: Transfer characteristics %d not implemented", m_nclx->transfer_characteristics );
        break;
    }
}
