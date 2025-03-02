#include <string.h>
#include <tiffio.h>

#include "TiffLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/FileWrapper.hpp"

TiffLoader::TiffLoader( std::shared_ptr<FileWrapper> file )
    : m_file( std::move( file ) )
    , m_tiff( nullptr )
{
    fseek( *m_file, 0, SEEK_SET );
    uint8_t hdr[4];
    if( fread( hdr, 1, 4, *m_file ) == 4 && ( memcmp( hdr, "II*\0", 4 ) == 0 || memcmp( hdr, "MM\0*", 4 ) == 0 ) )
    {
        fseek( *m_file, 0, SEEK_SET );
        fflush( *m_file );
        m_tiff = TIFFFdOpen( fileno( *m_file ), "<unknown>", "r" );
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

std::unique_ptr<Bitmap> TiffLoader::Load()
{
    uint32_t width, height;
    TIFFGetField( m_tiff, TIFFTAG_IMAGEWIDTH, &width );
    TIFFGetField( m_tiff, TIFFTAG_IMAGELENGTH, &height );

    auto bmp = std::make_unique<Bitmap>( width, height );

    if( TIFFReadRGBAImageOriented( m_tiff, width, height, (uint32_t*)bmp->Data(), ORIENTATION_TOPLEFT ) == 0 )
    {
        return nullptr;
    }

    return bmp;
}
