#include <stdio.h>
#include <jpeglib.h>
#include <lcms2.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include "JpgLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/Cmyk.hpp"
#include "util/FileWrapper.hpp"
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

    JOCTET* icc = nullptr;

    jpeg_decompress_struct cinfo;
    JpgErrorMgr jerr;

    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = []( j_common_ptr cinfo ) { longjmp( ((JpgErrorMgr*)cinfo->err)->setjmp_buffer, 1 ); };
    if( setjmp( jerr.setjmp_buffer ) )
    {
        free( icc );
        jpeg_destroy_decompress( &cinfo );
        return nullptr;
    }

    jpeg_create_decompress( &cinfo );
    jpeg_stdio_src( &cinfo, *m_file );
    jpeg_save_markers( &cinfo, JPEG_APP0 + 2, 0xFFFF );
    jpeg_read_header( &cinfo, TRUE );
    const bool cmyk = cinfo.jpeg_color_space == JCS_CMYK || cinfo.jpeg_color_space == JCS_YCCK;
    if( !cmyk ) cinfo.out_color_space = JCS_EXT_RGBX;
    jpeg_start_decompress( &cinfo );

    auto bmp = std::make_unique<Bitmap>( cinfo.output_width, cinfo.output_height );
    auto ptr = bmp->Data();

    unsigned int iccSz;
    jpeg_read_icc_profile( &cinfo, &icc, &iccSz );

    while( cinfo.output_scanline < cinfo.output_height )
    {
        jpeg_read_scanlines( &cinfo, &ptr, 1 );
        ptr += cinfo.output_width * 4;
    }

    if( cmyk )
    {
        cmsHPROFILE profileIn;

        if( icc )
        {
            mclog( LogLevel::Info, "ICC profile size: %u", iccSz );
            profileIn = cmsOpenProfileFromMem( icc, iccSz );
        }
        else
        {
            mclog( LogLevel::Info, "No ICC profile found, using default" );
            profileIn = cmsOpenProfileFromMem( CMYK::ProfileData(), CMYK::ProfileSize() );
        }

        auto profileOut = cmsCreate_sRGBProfile();
        auto transform = cmsCreateTransform( profileIn, TYPE_CMYK_8_REV, profileOut, TYPE_RGBA_8, INTENT_PERCEPTUAL, 0 );

        if( transform )
        {
            cmsDoTransform( transform, bmp->Data(), bmp->Data(), bmp->Width() * bmp->Height() );
            cmsDeleteTransform( transform );
        }

        cmsCloseProfile( profileOut );
        cmsCloseProfile( profileIn );
    }
    else if( icc )
    {
        mclog( LogLevel::Info, "ICC profile size: %u", iccSz );

        auto profileIn = cmsOpenProfileFromMem( icc, iccSz );
        auto profileOut = cmsCreate_sRGBProfile();
        auto transform = cmsCreateTransform( profileIn, TYPE_RGBA_8, profileOut, TYPE_RGBA_8, INTENT_PERCEPTUAL, 0 );

        if( transform )
        {
            cmsDoTransform( transform, bmp->Data(), bmp->Data(), bmp->Width() * bmp->Height() );
            cmsDeleteTransform( transform );
        }

        cmsCloseProfile( profileOut );
        cmsCloseProfile( profileIn );
    }

    free( icc );
    jpeg_finish_decompress( &cinfo );
    jpeg_destroy_decompress( &cinfo );

    bmp->SetAlpha( 0xFF );
    return bmp;
}
