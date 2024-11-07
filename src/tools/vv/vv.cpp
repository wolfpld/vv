#include <getopt.h>
#include <memory>
#include <sys/ioctl.h>

#include "image/ImageLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/Callstack.hpp"
#include "util/Logs.hpp"
#include "util/Panic.hpp"

int main( int argc, char** argv )
{
#ifdef NDEBUG
    SetLogLevel( LogLevel::Error );
#endif

    struct option longOptions[] = {
        { "debug", no_argument, nullptr, 'd' },
        { "external", no_argument, nullptr, 'e' },
    };

    int opt;
    while( ( opt = getopt_long( argc, argv, "de", longOptions, nullptr ) ) != -1 )
    {
        switch (opt)
        {
        case 'd':
            SetLogLevel( LogLevel::Callstack );
            break;
        case 'e':
            ShowExternalCallstacks( true );
            break;
        default:
            break;
        }
    }
    if (optind == argc)
    {
        mclog( LogLevel::Error, "Image file name must be provided" );
        return 1;
    }

    struct winsize ws;
    ioctl( 0, TIOCGWINSZ, &ws );
    mclog( LogLevel::Info, "Terminal size: %dx%d", ws.ws_col, ws.ws_row );

    const char* imageFile = argv[optind];
    auto bitmap = std::unique_ptr<Bitmap>( LoadImage( imageFile ) );
    CheckPanic( bitmap, "Failed to load image" );
    mclog( LogLevel::Info, "Image loaded: %ux%u", bitmap->Width(), bitmap->Height() );

    uint32_t col = ws.ws_col;
    uint32_t row = std::max<uint16_t>( 1, ws.ws_row - 1 ) * 2;

    mclog( LogLevel::Info, "Virtual pixels: %ux%u", col, row );

    if( bitmap->Width() > col || bitmap->Height() > row )
    {
        const auto ratio = std::min( float( col ) / bitmap->Width(), float( row ) / bitmap->Height() );
        bitmap->Resize( bitmap->Width() * ratio, bitmap->Height() * ratio );
        mclog( LogLevel::Info, "Image resized: %ux%u", bitmap->Width(), bitmap->Height() );
    }

    auto px0 = (uint32_t*)bitmap->Data();
    auto px1 = px0 + bitmap->Width();

    for( int y=0; y<bitmap->Height() / 2; y++ )
    {
        for( int x=0; x<bitmap->Width(); x++ )
        {
            auto c0 = *px0++;
            auto c1 = *px1++;
            auto r0 = ( c0       ) & 0xFF;
            auto g0 = ( c0 >> 8  ) & 0xFF;
            auto b0 = ( c0 >> 16 ) & 0xFF;
            auto r1 = ( c1       ) & 0xFF;
            auto g1 = ( c1 >> 8  ) & 0xFF;
            auto b1 = ( c1 >> 16 ) & 0xFF;
            printf( "\033[38;2;%d;%d;%dm\033[48;2;%d;%d;%dm▀", r0, g0, b0, r1, g1, b1 );
        }
        printf( "\033[0m\n" );
        px0 += bitmap->Width();
        px1 += bitmap->Width();
    }
    if( ( bitmap->Height() & 1 ) != 0 )
    {
        for( int x=0; x<bitmap->Width(); x++ )
        {
            auto c0 = *px0++;
            auto r0 = ( c0       ) & 0xFF;
            auto g0 = ( c0 >> 8  ) & 0xFF;
            auto b0 = ( c0 >> 16 ) & 0xFF;
            printf( "\033[38;2;%d;%d;%dm▀", r0, g0, b0 );
        }
        printf( "\033[0m\n" );
    }

    return 0;
}
