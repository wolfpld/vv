#include <cairo.h>
#include <dlfcn.h>
#include <glib-object.h>
#include <stdint.h>
#include <string.h>

#include "PdfImage.hpp"
#include "util/Bitmap.hpp"
#include "util/Panic.hpp"

typedef void*(*LoadPdf_t)( int, const char*, GError** );
typedef void*(*GetPage_t)( void*, int );
typedef void(*GetPageSize_t)( void*, double*, double* );
typedef void(*RenderPage_t)( void*, cairo_t* );

LoadPdf_t LoadPdf = nullptr;
GetPage_t GetPage = nullptr;
GetPageSize_t GetPageSize = nullptr;
RenderPage_t RenderPage = nullptr;

struct PdfLibraryLoader
{
    PdfLibraryLoader()
    {
        auto lib = dlopen( "libpoppler-glib.so", RTLD_LAZY );
        if( lib )
        {
            auto LoadPdf_f = (LoadPdf_t)dlsym( lib, "poppler_document_new_from_fd" );
            auto GetPage_f = (GetPage_t)dlsym( lib, "poppler_document_get_page" );
            auto GetPageSize_f = (GetPageSize_t)dlsym( lib, "poppler_page_get_size" );
            auto RenderPage_f = (RenderPage_t)dlsym( lib, "poppler_page_render_for_printing" );

            if( LoadPdf_f && GetPage_f && GetPageSize_f && RenderPage_f )
            {
                LoadPdf = LoadPdf_f;
                GetPage = GetPage_f;
                GetPageSize = GetPageSize_f;
                RenderPage = RenderPage_f;
            }
            else
            {
                dlclose( lib );
            }
        }
    };
};

PdfImage::PdfImage( FileWrapper& file )
    : m_pdf( nullptr )
    , m_page( nullptr )
{
    fseek( file, 0, SEEK_SET );
    uint8_t hdr[5];
    if( fread( hdr, 1, 5, file ) == 5 && memcmp( hdr, "%PDF-", 5 ) == 0 )
    {
        static PdfLibraryLoader loader;
        if( LoadPdf )
        {
            m_pdf = LoadPdf( fileno( file ), nullptr, nullptr );
            if( m_pdf )
            {
                file.Release();

                m_page = GetPage( m_pdf, 0 );
                CheckPanic( m_page, "Failed to load PDF page" );

                double w, h;
                GetPageSize( m_page, &w, &h );

                m_width = w;
                m_height = h;
            }
        }
    }
}

PdfImage::~PdfImage()
{
    if( m_page ) g_object_unref( m_page );
    if( m_pdf ) g_object_unref( m_pdf );
}

bool PdfImage::IsValid() const
{
    return m_pdf != nullptr;
}

std::unique_ptr<Bitmap> PdfImage::Rasterize( int width, int height )
{
    CheckPanic( m_page, "Invalid PDF image" );

    const auto stride = cairo_format_stride_for_width( CAIRO_FORMAT_ARGB32, width );
    auto data = new uint8_t[height * stride];
    memset( data, 0, height * stride );
    auto surface = cairo_image_surface_create_for_data( data, CAIRO_FORMAT_ARGB32, width, height, stride );
    auto cr = cairo_create( surface );
    cairo_scale( cr, float( width ) / m_width, float( height ) / m_height );

    RenderPage( m_page, cr );

    auto img = std::make_unique<Bitmap>( width, height );
    auto dst = (uint32_t*)img->Data();
    auto src = data;

    while( height-- )
    {
        for( int x = 0; x < width; x++ )
        {
            uint32_t px = *(uint32_t*)src;
            *dst++ = ( px & 0xFF00FF00 ) | ( px & 0x00FF0000 ) >> 16 | ( px & 0x000000FF ) << 16;
            src += 4;
        }
        src += stride - width * 4;
    }

    cairo_destroy( cr );
    cairo_surface_destroy( surface );
    delete[] data;

    return img;
}
