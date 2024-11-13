#include <string.h>

#define DR_PCX_IMPLEMENTATION
#define DR_PCX_NO_STDIO
#include "contrib/dr_pcx.h"

#include "PcxLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/Panic.hpp"

PcxLoader::PcxLoader( FileWrapper& file )
    : m_file( file )
    , m_valid( false )
{
    fseek( m_file, 0, SEEK_SET );

    struct {
        uint8_t magic;
        uint8_t version;
        uint8_t encoding;
        uint8_t bpp;
    } header;

    if( fread( &header, 1, sizeof( header ), m_file ) != sizeof( header ) ) return;
    if( header.magic != 0x0A ) return;
    if( header.version == 1 || header.version > 5 ) return;
    if( header.encoding != 1 ) return;
    if( header.bpp != 1 && header.bpp != 2 && header.bpp != 4 && header.bpp != 8 ) return;

    m_valid = true;
}

bool PcxLoader::IsValid() const
{
    return m_valid;
}

Bitmap* PcxLoader::Load()
{
    CheckPanic( m_valid, "Invalid PCX file" );

    fseek( m_file, 0, SEEK_SET );

    int w, h, comp;
    auto data = drpcx_load( []( void* f, void* out, size_t sz ) { return fread( out, 1, sz, (FILE*)f ); }, (FILE*)m_file, false, &w, &h, &comp, 4 );
    if( data == nullptr ) return nullptr;

    auto bmp = new Bitmap( w, h );
    memcpy( bmp->Data(), data, w * h * 4 );

    drpcx_free( data );
    return bmp;
}
