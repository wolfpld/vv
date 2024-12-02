#include <vector>

#include <ImfRgbaFile.h>

#include "ExrLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/BitmapHdr.hpp"
#include "util/Panic.hpp"

class ExrStream : public Imf::IStream
{
public:
    explicit ExrStream( FILE* f )
        : Imf::IStream( "<unknown>" )
        , m_file( f )
    {
        fseek( m_file, 0, SEEK_SET );
    }

    bool isMemoryMapped() const override { return false; }
    bool read( char c[], int n ) override { fread( c, 1, n, m_file ); return !feof( m_file ); }
    uint64_t tellg() override { return ftell( m_file ); }
    void seekg( uint64_t pos ) override { fseek( m_file, pos, SEEK_SET ); }

private:
    FILE* m_file;
};

ExrLoader::ExrLoader( std::shared_ptr<FileWrapper> file )
    : ImageLoader( std::move( file ) )
{
    try
    {
        m_stream = std::make_unique<ExrStream>( *m_file );
        m_exr = std::make_unique<Imf::RgbaInputFile>( *m_stream );
        m_valid = true;
    }
    catch( const std::exception& )
    {
        m_valid = false;
    }
}

ExrLoader::~ExrLoader()
{
}

bool ExrLoader::IsValid() const
{
    return m_valid;
}

std::unique_ptr<Bitmap> ExrLoader::Load()
{
    auto hdr = LoadHdr();
    return hdr->Tonemap();
}

std::unique_ptr<BitmapHdr> ExrLoader::LoadHdr()
{
    CheckPanic( m_exr, "Invalid EXR file" );

    auto dw = m_exr->dataWindow();
    auto width = dw.max.x - dw.min.x + 1;
    auto height = dw.max.y - dw.min.y + 1;

    std::vector<Imf::Rgba> hdr;
    hdr.resize( width * height );

    m_exr->setFrameBuffer( hdr.data(), 1, width );
    m_exr->readPixels( dw.min.y, dw.max.y );

    auto bmp = std::make_unique<BitmapHdr>( width, height );
    auto dst = bmp->Data();
    auto src = hdr.data();
    auto sz = width * height;
    do
    {
        *dst++ = src->r;
        *dst++ = src->g;
        *dst++ = src->b;
        *dst++ = src->a;
        src++;
    }
    while( --sz );

    return bmp;
}
