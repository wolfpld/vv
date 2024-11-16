#include <cairo.h>
#include <glib.h>
#include <librsvg/rsvg.h>
#include <math.h>
#include <string.h>

#include "SvgImage.hpp"
#include "util/Bitmap.hpp"
#include "util/FileBuffer.hpp"
#include "util/Logs.hpp"
#include "util/Panic.hpp"

SvgImage::SvgImage( FileWrapper& file, const char* path )
    : m_buf( std::make_unique<FileBuffer>( file ) )
    , m_file( g_file_new_for_path( path ) )
    , m_stream( g_memory_input_stream_new_from_data( m_buf->data(), m_buf->size(), nullptr ) )
    , m_handle( rsvg_handle_new_from_stream_sync( m_stream, nullptr, RSVG_HANDLE_FLAGS_NONE, nullptr, nullptr ) )
{
    if( m_handle )
    {
        rsvg_handle_set_dpi( m_handle, 96 );

        double w, h;
        if( rsvg_handle_get_intrinsic_size_in_pixels( m_handle, &w, &h ) )
        {
            m_width = ceil( w );
            m_height = ceil( h );
        }
    }
}

SvgImage::~SvgImage()
{
    if( m_handle ) g_object_unref( m_handle );
    g_object_unref( m_stream );
    g_object_unref( m_file );
}

bool SvgImage::IsValid() const
{
    return m_handle != nullptr;
}

Bitmap* SvgImage::Rasterize( int width, int height )
{
    CheckPanic( m_handle, "Invalid SVG image" );
    if( !m_handle ) return nullptr;

    const auto stride = cairo_format_stride_for_width( CAIRO_FORMAT_ARGB32, width );
    auto data = new uint8_t[height * stride];
    memset( data, 0, height * stride );
    auto surface = cairo_image_surface_create_for_data( data, CAIRO_FORMAT_ARGB32, width, height, stride );
    auto cr = cairo_create( surface );

    RsvgRectangle viewbox = { 0, 0, double( width ), double( height ) };
    if( !rsvg_handle_render_document( m_handle, cr, &viewbox, nullptr ) )
    {
        cairo_destroy( cr );
        cairo_surface_destroy( surface );
        delete[] data;
        return nullptr;
    }

    Bitmap* img = new Bitmap( width, height );
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
