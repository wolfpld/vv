#include <jxl/decode.h>
#include <jxl/resizable_parallel_runner.h>

#include "JxlLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/FileBuffer.hpp"
#include "util/Panic.hpp"

JxlLoader::JxlLoader( std::shared_ptr<FileWrapper> file )
    : ImageLoader( std::move( file ) )
{
    fseek( *m_file, 0, SEEK_SET );
    uint8_t hdr[12];
    const auto read = fread( hdr, 1, 12, *m_file );
    const auto res = JxlSignatureCheck( hdr, read );
    m_valid = res == JXL_SIG_CODESTREAM || res == JXL_SIG_CONTAINER;
}

bool JxlLoader::IsValid() const
{
    return m_valid;
}

std::unique_ptr<Bitmap> JxlLoader::Load()
{
    CheckPanic( m_valid, "Invalid JPEG XL file" );

    FileBuffer buf( m_file );

    auto runner = JxlResizableParallelRunnerCreate( nullptr );

    auto dec = JxlDecoderCreate( nullptr );
    JxlDecoderSubscribeEvents( dec, JXL_DEC_BASIC_INFO | JXL_DEC_FULL_IMAGE );
    JxlDecoderSetParallelRunner( dec, JxlResizableParallelRunner, runner );

    JxlDecoderSetInput( dec, (const uint8_t*)buf.data(), buf.size() );
    JxlDecoderCloseInput( dec );

    std::unique_ptr<Bitmap> bmp;
    for(;;)
    {
        const auto res = JxlDecoderProcessInput( dec );
        if( res == JXL_DEC_ERROR || res == JXL_DEC_NEED_MORE_INPUT )
        {
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

            bmp = std::make_unique<Bitmap>( info.xsize, info.ysize );

            JxlPixelFormat format = { 4, JXL_TYPE_UINT8, JXL_LITTLE_ENDIAN, 0 };
            if( JxlDecoderSetImageOutBuffer( dec, &format, bmp->Data(), bmp->Width() * bmp->Height() * 4 ) != JXL_DEC_SUCCESS )
            {
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
