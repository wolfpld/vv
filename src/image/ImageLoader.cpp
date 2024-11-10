#include <concepts>
#include <tracy/Tracy.hpp>

#include "DdsLoader.hpp"
#include "ExrLoader.hpp"
#include "HeifLoader.hpp"
#include "ImageLoader.hpp"
#include "JpgLoader.hpp"
#include "JxlLoader.hpp"
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

template<typename T>
concept ImageLoader = requires( T loader, FileWrapper& file )
{
    { loader.IsValid() } -> std::convertible_to<bool>;
    { loader.Load() } -> std::convertible_to<Bitmap*>;
};

template<ImageLoader T>
static inline Bitmap* LoadImage( FileWrapper& file )
{
    T loader( file );
    if( !loader.IsValid() ) return nullptr;
    return loader.Load();
}

Bitmap* LoadImage( const char* filename )
{
    ZoneScoped;

    auto path = ExpandHome( filename );

    FileWrapper file( path.c_str(), "rb" );
    if( !file )
    {
        mclog( LogLevel::Error, "Image %s does not exist.", path.c_str() );
        return nullptr;
    }

    mclog( LogLevel::Info, "Loading image %s", path.c_str() );

    if( auto img = LoadImage<PngLoader>( file ); img ) return img;
    if( auto img = LoadImage<JpgLoader>( file ); img ) return img;
    if( auto img = LoadImage<JxlLoader>( file ); img ) return img;
    if( auto img = LoadImage<WebpLoader>( file ); img ) return img;
    if( auto img = LoadImage<HeifLoader>( file ); img ) return img;
    if( auto img = LoadImage<PvrLoader>( file ); img ) return img;
    if( auto img = LoadImage<DdsLoader>( file ); img ) return img;
    if( auto img = LoadImage<StbImageLoader>( file ); img ) return img;
    if( auto img = LoadImage<RawLoader>( file ); img ) return img;
    if( auto img = LoadImage<TiffLoader>( file ); img ) return img;
    if( auto img = LoadImage<ExrLoader>( file ); img ) return img;

    mclog( LogLevel::Error, "Failed to load image %s", path.c_str() );
    return nullptr;
}
