#include <jxl/decode.h>
#include <jxl/resizable_parallel_runner.h>
#include <vector>

#include "JxlLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/Panic.hpp"

JxlLoader::JxlLoader( FileWrapper& file )
    : m_file( file )
{
    fseek( m_file, 0, SEEK_SET );
    uint8_t hdr[12];
    const auto read = fread( hdr, 1, 12, m_file );
    const auto res = JxlSignatureCheck( hdr, read );
    m_valid = res == JXL_SIG_CODESTREAM || res == JXL_SIG_CONTAINER;
}

bool JxlLoader::IsValid() const
{
    return m_valid;
}

Bitmap* JxlLoader::Load()
{
    CheckPanic( m_valid, "Invalid JPEG XL file" );

    fseek( m_file, 0, SEEK_END );
    const auto sz = ftell( m_file );
    fseek( m_file, 0, SEEK_SET );

    std::vector<uint8_t> buf( sz );
    fread( buf.data(), 1, sz, m_file );

    Bitmap* bmp = nullptr;

    auto runner = JxlResizableParallelRunnerCreate( nullptr );

    auto dec = JxlDecoderCreate( nullptr );
    JxlDecoderSubscribeEvents( dec, JXL_DEC_BASIC_INFO | JXL_DEC_FULL_IMAGE );
    JxlDecoderSetParallelRunner( dec, JxlResizableParallelRunner, runner );

    JxlDecoderSetInput( dec, buf.data(), buf.size() );
    JxlDecoderCloseInput( dec );

    for(;;)
    {
        const auto res = JxlDecoderProcessInput( dec );
        if( res == JXL_DEC_ERROR || res == JXL_DEC_NEED_MORE_INPUT )
        {
            delete bmp;
            JxlDecoderDestroy( dec );
            JxlResizableParallelRunnerDestroy( runner );
            return nullptr;
        }
        if( res == JXL_DEC_SUCCESS || res == JXL_DEC_FULL_IMAGE ) break;
        if( res == JXL_DEC_BASIC_INFO )
        {
            JxlBasicInfo info;
            JxlDecoderGetBasicInfo( dec, &info );
            JxlResizableParallelRunnerSetThreads( runner, JxlResizableParallelRunnerSuggestThreads( info.xsize, info.ysize ) );

            bmp = new Bitmap( info.xsize, info.ysize );

            JxlPixelFormat format = { 4, JXL_TYPE_UINT8, JXL_LITTLE_ENDIAN, 0 };
            if( JxlDecoderSetImageOutBuffer( dec, &format, bmp->Data(), bmp->Width() * bmp->Height() * 4 ) != JXL_DEC_SUCCESS )
            {
                delete bmp;
                JxlDecoderDestroy( dec );
                JxlResizableParallelRunnerDestroy( runner );
                return nullptr;
            }
        }
    }

    JxlDecoderDestroy( dec );
    JxlResizableParallelRunnerDestroy( runner );
    return bmp;
}
