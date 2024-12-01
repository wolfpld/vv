#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <stdint.h>

#include "JpgLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/Panic.hpp"

JpgLoader::JpgLoader( std::shared_ptr<FileWrapper> file )
    : ImageLoader( std::move( file ) )
{
    fseek( *m_file, 0, SEEK_SET );
    uint8_t hdr[2];
    m_valid = fread( hdr, 1, 2, *m_file ) == 2 && hdr[0] == 0xFF && hdr[1] == 0xD8;
}

bool JpgLoader::IsValid() const
{
    return m_valid;
}

struct JpgErrorMgr
{
    jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

std::unique_ptr<Bitmap> JpgLoader::Load()
{
    CheckPanic( m_valid, "Invalid JPEG file" );
    fseek( *m_file, 0, SEEK_SET );

    jpeg_decompress_struct cinfo;
    JpgErrorMgr jerr;

    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = []( j_common_ptr cinfo ) { longjmp( ((JpgErrorMgr*)cinfo->err)->setjmp_buffer, 1 ); };
    if( setjmp( jerr.setjmp_buffer ) )
    {
        jpeg_destroy_decompress( &cinfo );
        return nullptr;
    }

    jpeg_create_decompress( &cinfo );
    jpeg_stdio_src( &cinfo, *m_file );
    jpeg_read_header( &cinfo, TRUE );
    cinfo.out_color_space = JCS_EXT_RGBA;
    jpeg_start_decompress( &cinfo );

    auto bmp = std::make_unique<Bitmap>( cinfo.output_width, cinfo.output_height );
    auto ptr = bmp->Data();
    while( cinfo.output_scanline < cinfo.output_height )
    {
        jpeg_read_scanlines( &cinfo, &ptr, 1 );
        ptr += cinfo.output_width * 4;
    }

    jpeg_finish_decompress( &cinfo );
    jpeg_destroy_decompress( &cinfo );
    return bmp;
}
