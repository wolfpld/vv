#include <string.h>
#include <tiffio.h>

#include "TiffLoader.hpp"
#include "util/Bitmap.hpp"

TiffLoader::TiffLoader( FileWrapper& file )
    : m_tiff( nullptr )
{
    fseek( file, 0, SEEK_SET );
    uint8_t hdr[4];
    if( fread( hdr, 1, 4, file ) == 4 && ( memcmp( hdr, "II*\0", 4 ) == 0 || memcmp( hdr, "MM\0*", 4 ) == 0 ) )
    {
        fseek( file, 0, SEEK_SET );
        fflush( file );
        m_tiff = TIFFFdOpen( fileno( file ), "<unknown>", "r" );
    }
}

TiffLoader::~TiffLoader()
{
    if( m_tiff ) TIFFClose( m_tiff );
}

bool TiffLoader::IsValid() const
{
    return m_tiff != nullptr;
}

Bitmap* TiffLoader::Load()
{
    uint32_t width, height;
    TIFFGetField( m_tiff, TIFFTAG_IMAGEWIDTH, &width );
    TIFFGetField( m_tiff, TIFFTAG_IMAGELENGTH, &height );

    auto bmp = new Bitmap( width, height );

    if( TIFFReadRGBAImageOriented( m_tiff, width, height, (uint32_t*)bmp->Data(), ORIENTATION_TOPLEFT ) == 0 )
    {
        delete bmp;
        return nullptr;
    }

    return bmp;
}