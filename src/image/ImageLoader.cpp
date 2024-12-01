#include <concepts>
#include <tracy/Tracy.hpp>

#include "DdsLoader.hpp"
#include "ExrLoader.hpp"
#include "HeifLoader.hpp"
#include "ImageLoader.hpp"
#include "JpgLoader.hpp"
#include "JxlLoader.hpp"
#include "PcxLoader.hpp"
#include "PngLoader.hpp"
#include "PvrLoader.hpp"
#include "RawLoader.hpp"
#include "StbImageLoader.hpp"
#include "TiffLoader.hpp"
#include "WebpLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/FileWrapper.hpp"
#include "util/Home.hpp"
#include "util/Logs.hpp"
#include "vector/PdfImage.hpp"
#include "vector/SvgImage.hpp"

template<typename T>
concept ImageLoaderConcept = requires( T loader, const std::shared_ptr<FileWrapper>& file )
{
    { loader.IsValid() } -> std::convertible_to<bool>;
    { loader.Load() } -> std::convertible_to<std::unique_ptr<Bitmap>>;
};

template<ImageLoaderConcept T>
static inline std::unique_ptr<ImageLoader> CheckImageLoader( const std::shared_ptr<FileWrapper>& file )
{
    auto loader = std::make_unique<T>( file );
    if( loader->IsValid() ) return loader;
    return nullptr;
}

std::unique_ptr<ImageLoader> GetImageLoader( const char* filename )
{
    ZoneScoped;

    auto path = ExpandHome( filename );
    auto file = std::make_shared<FileWrapper>( path.c_str(), "rb" );
    if( !file )
    {
        mclog( LogLevel::Error, "Image %s does not exist.", path.c_str() );
        return nullptr;
    }

    if( auto loader = CheckImageLoader<PngLoader>( file ); loader ) return loader;
    if( auto loader = CheckImageLoader<JpgLoader>( file ); loader ) return loader;
    if( auto loader = CheckImageLoader<JxlLoader>( file ); loader ) return loader;
    if( auto loader = CheckImageLoader<WebpLoader>( file ); loader ) return loader;
    if( auto loader = CheckImageLoader<HeifLoader>( file ); loader ) return loader;
    if( auto loader = CheckImageLoader<PvrLoader>( file ); loader ) return loader;
    if( auto loader = CheckImageLoader<DdsLoader>( file ); loader ) return loader;
    if( auto loader = CheckImageLoader<StbImageLoader>( file ); loader ) return loader;
    if( auto loader = CheckImageLoader<RawLoader>( file ); loader ) return loader;
    if( auto loader = CheckImageLoader<TiffLoader>( file ); loader ) return loader;
    if( auto loader = CheckImageLoader<ExrLoader>( file ); loader ) return loader;
    if( auto loader = CheckImageLoader<PcxLoader>( file ); loader ) return loader;

    mclog( LogLevel::Info, "Raster image loaders can't open %s", path.c_str() );
    return nullptr;
}

std::unique_ptr<Bitmap> LoadImage( const char* filename )
{
    ZoneScoped;
    mclog( LogLevel::Info, "Loading image %s", filename );

    auto loader = GetImageLoader( filename );
    if( loader ) return loader->Load();
    return nullptr;
}

std::unique_ptr<VectorImage> LoadVectorImage( const char* filename )
{
    ZoneScoped;

    auto path = ExpandHome( filename );

    FileWrapper file( path.c_str(), "rb" );
    if( !file )
    {
        mclog( LogLevel::Error, "Vector image %s does not exist.", path.c_str() );
        return nullptr;
    }

    mclog( LogLevel::Info, "Loading vector image %s", path.c_str() );

    if( auto img = std::make_unique<SvgImage>( file, path.c_str() ); img->IsValid() ) return img;
    if( auto img = std::make_unique<PdfImage>( file, path.c_str() ); img->IsValid() ) return img;

    mclog( LogLevel::Info, "Vector loaders can't open %s", path.c_str() );
    return nullptr;
}
