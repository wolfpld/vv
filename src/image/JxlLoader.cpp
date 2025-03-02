#include <jxl/cms_interface.h>
#include <jxl/color_encoding.h>
#include <jxl/decode.h>
#include <jxl/resizable_parallel_runner.h>
#include <lcms2.h>

#include "JxlLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/BitmapHdr.hpp"
#include "util/FileBuffer.hpp"
#include "util/FileWrapper.hpp"
#include "util/Panic.hpp"

namespace
{
constexpr JxlColorEncoding srgb = {
    .color_space = JXL_COLOR_SPACE_RGB,
    .white_point = JXL_WHITE_POINT_D65,
    .primaries = JXL_PRIMARIES_SRGB,
    .transfer_function = JXL_TRANSFER_FUNCTION_SRGB,
    .rendering_intent = JXL_RENDERING_INTENT_PERCEPTUAL
};

constexpr JxlColorEncoding linear = {
    .color_space = JXL_COLOR_SPACE_RGB,
    .white_point = JXL_WHITE_POINT_D65,
    .primaries = JXL_PRIMARIES_SRGB,
    .transfer_function = JXL_TRANSFER_FUNCTION_LINEAR,
    .rendering_intent = JXL_RENDERING_INTENT_PERCEPTUAL
};

void* CmsInit( void* data, size_t num_threads, size_t pixels_per_thread, const JxlColorProfile* input_profile, const JxlColorProfile* output_profile, float intensity_target )
{
    auto cms = (JxlLoader::CmsData*)data;

    cms->srcBuf.resize( num_threads );
    cms->dstBuf.resize( num_threads );

    for( size_t i=0; i<num_threads; i++ )
    {
        cms->srcBuf[i] = new float[pixels_per_thread * 3];
        cms->dstBuf[i] = new float[pixels_per_thread * 3];
    }

    cms->profileIn = cmsOpenProfileFromMem( input_profile->icc.data, input_profile->icc.size );
    cms->profileOut = cmsOpenProfileFromMem( output_profile->icc.data, output_profile->icc.size );
    cms->transform = cmsCreateTransform( cms->profileIn, TYPE_RGB_FLT, cms->profileOut, TYPE_RGB_FLT, INTENT_PERCEPTUAL, 0 );

    return cms;
}

float* CmsGetSrcBuffer( void* data, size_t thread )
{
    auto cms = (JxlLoader::CmsData*)data;
    return cms->srcBuf[thread];
}

float* CmsGetDstBuffer( void* data, size_t thread )
{
    auto cms = (JxlLoader::CmsData*)data;
    return cms->dstBuf[thread];
}

JXL_BOOL CmsRun( void* data, size_t thread, const float* input, float* output, size_t num_pixels )
{
    auto cms = (JxlLoader::CmsData*)data;
    cmsDoTransform( cms->transform, input, output, num_pixels );
    return true;
}

void CmsDestroy( void* data )
{
    auto cms = (JxlLoader::CmsData*)data;

    cmsDeleteTransform( cms->transform );
    cmsCloseProfile( cms->profileOut );
    cmsCloseProfile( cms->profileIn );

    for( auto& buf : cms->srcBuf ) delete[] buf;
    for( auto& buf : cms->dstBuf ) delete[] buf;
}
}

JxlLoader::JxlLoader( std::shared_ptr<FileWrapper> file )
    : m_file( std::move( file ) )
    , m_runner( nullptr )
    , m_dec( nullptr )
{
    fseek( *m_file, 0, SEEK_SET );
    uint8_t hdr[12];
    const auto read = fread( hdr, 1, 12, *m_file );
    const auto res = JxlSignatureCheck( hdr, read );
    m_valid = res == JXL_SIG_CODESTREAM || res == JXL_SIG_CONTAINER;
}

JxlLoader::~JxlLoader()
{
    if( m_dec ) JxlDecoderDestroy( m_dec );
    if( m_runner ) JxlResizableParallelRunnerDestroy( m_runner );
}

bool JxlLoader::IsValid() const
{
    return m_valid;
}

bool JxlLoader::IsHdr()
{
    if( !m_dec && !Open() ) return false;
    return m_info.bits_per_sample > 8;
}

bool JxlLoader::PreferHdr()
{
    return true;
}

std::unique_ptr<Bitmap> JxlLoader::Load()
{
    if( !m_dec && !Open() ) return nullptr;

    auto bmp = std::make_unique<Bitmap>( m_info.xsize, m_info.ysize );

    JxlPixelFormat format = { 4, JXL_TYPE_UINT8, JXL_LITTLE_ENDIAN, 0 };
    if( JxlDecoderSetImageOutBuffer( m_dec, &format, bmp->Data(), bmp->Width() * bmp->Height() * 4 ) != JXL_DEC_SUCCESS ) return nullptr;

    for(;;)
    {
        const auto res = JxlDecoderProcessInput( m_dec );
        if( res == JXL_DEC_ERROR || res == JXL_DEC_NEED_MORE_INPUT || res == JXL_DEC_BASIC_INFO ) return nullptr;
        if( res == JXL_DEC_SUCCESS || res == JXL_DEC_FULL_IMAGE ) break;
        if( res == JXL_DEC_COLOR_ENCODING )
        {
            JxlDecoderSetOutputColorProfile( m_dec, &srgb, nullptr, 0 );
        }
    }

    return bmp;
}

std::unique_ptr<BitmapHdr> JxlLoader::LoadHdr()
{
    if( !m_dec && !Open() ) return nullptr;

    auto bmp = std::make_unique<BitmapHdr>( m_info.xsize, m_info.ysize );

    JxlPixelFormat format = { 4, JXL_TYPE_FLOAT, JXL_LITTLE_ENDIAN, 0 };
    if( JxlDecoderSetImageOutBuffer( m_dec, &format, bmp->Data(), bmp->Width() * bmp->Height() * 4 * sizeof( float ) ) != JXL_DEC_SUCCESS ) return nullptr;

    for(;;)
    {
        const auto res = JxlDecoderProcessInput( m_dec );
        if( res == JXL_DEC_ERROR || res == JXL_DEC_NEED_MORE_INPUT || res == JXL_DEC_BASIC_INFO ) return nullptr;
        if( res == JXL_DEC_SUCCESS || res == JXL_DEC_FULL_IMAGE ) break;
        if( res == JXL_DEC_COLOR_ENCODING )
        {
            JxlDecoderSetOutputColorProfile( m_dec, &linear, nullptr, 0 );
        }
    }

    return bmp;
}

bool JxlLoader::Open()
{
    CheckPanic( m_valid, "Invalid JPEG XL file" );
    CheckPanic( !m_buf && !m_runner && !m_dec, "Already opened" );

    m_buf = std::make_unique<FileBuffer>( m_file );
    m_runner = JxlResizableParallelRunnerCreate( nullptr );

    m_dec = JxlDecoderCreate( nullptr );
    JxlDecoderSubscribeEvents( m_dec, JXL_DEC_BASIC_INFO | JXL_DEC_COLOR_ENCODING | JXL_DEC_FULL_IMAGE );
    JxlDecoderSetParallelRunner( m_dec, JxlResizableParallelRunner, m_runner );

    JxlDecoderSetInput( m_dec, (const uint8_t*)m_buf->data(), m_buf->size() );
    JxlDecoderCloseInput( m_dec );

    for(;;)
    {
        const auto res = JxlDecoderProcessInput( m_dec );
        if( res == JXL_DEC_ERROR || res == JXL_DEC_NEED_MORE_INPUT || res == JXL_DEC_SUCCESS || res == JXL_DEC_FULL_IMAGE ) return false;
        if( res == JXL_DEC_BASIC_INFO )
        {
            JxlDecoderGetBasicInfo( m_dec, &m_info );
            JxlResizableParallelRunnerSetThreads( m_runner, JxlResizableParallelRunnerSuggestThreads( m_info.xsize, m_info.ysize ) );

            const JxlCmsInterface cmsInterface
            {
                .init_data = &m_cms,
                .init = CmsInit,
                .get_src_buf = CmsGetSrcBuffer,
                .get_dst_buf = CmsGetDstBuffer,
                .run = CmsRun,
                .destroy = CmsDestroy
            };
            JxlDecoderSetCms( m_dec, cmsInterface );

            return true;
        }
    }
}
