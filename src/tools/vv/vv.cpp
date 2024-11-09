#include <libbase64.h>
#include <format>
#include <getopt.h>
#include <memory>
#include <thread>
#include <stdio.h>
#include <sys/ioctl.h>
#include <vector>
#include <zlib.h>

#include "Terminal.hpp"
#include "image/ImageLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/Callstack.hpp"
#include "util/Logs.hpp"
#include "util/Panic.hpp"

namespace {
void PrintHelp()
{
    printf( "Usage: vv [options] <image>\n" );
    printf( "Options:\n" );
    printf( "  -b, --block                  Use text-only block mode\n" );
    printf( "  -s, --scale                  Try to scale up image to 2x\n" );
    printf( "  -f, --fit                    Fit image to terminal size\n" );
    printf( "  --help                       Print this help\n" );
}

enum class ScaleMode
{
    None,
    Fit,
    Scale2x,
};

void AdjustBitmap( Bitmap& bitmap, uint32_t col, uint32_t row, ScaleMode scale )
{
    if( scale == ScaleMode::Fit || bitmap.Width() > col || bitmap.Height() > row )
    {
        const auto ratio = std::min( float( col ) / bitmap.Width(), float( row ) / bitmap.Height() );
        bitmap.Resize( bitmap.Width() * ratio, bitmap.Height() * ratio );
        mclog( LogLevel::Info, "Image resized: %ux%u", bitmap.Width(), bitmap.Height() );
    }
    else if( scale == ScaleMode::Scale2x && bitmap.Width() * 2 <= col && bitmap.Height() * 2 <= row )
    {
        bitmap.Resize( bitmap.Width() * 2, bitmap.Height() * 2 );
        mclog( LogLevel::Info, "Image upscaled: %ux%u", bitmap.Width(), bitmap.Height() );
    }
}
}

int main( int argc, char** argv )
{
#ifdef NDEBUG
    SetLogLevel( LogLevel::Error );
#endif

    enum { OptHelp };

    struct option longOptions[] = {
        { "debug", no_argument, nullptr, 'd' },
        { "external", no_argument, nullptr, 'e' },
        { "block", no_argument, nullptr, 'b' },
        { "scale", no_argument, nullptr, 's' },
        { "fit", no_argument, nullptr, 'f' },
        { "help", no_argument, nullptr, OptHelp },
    };

    bool blockMode = false;
    ScaleMode scale = ScaleMode::None;

    int opt;
    while( ( opt = getopt_long( argc, argv, "debsf", longOptions, nullptr ) ) != -1 )
    {
        switch (opt)
        {
        case 'd':
            SetLogLevel( LogLevel::Callstack );
            break;
        case 'e':
            ShowExternalCallstacks( true );
            break;
        case 'b':
            blockMode = true;
            break;
        case 's':
            scale = ScaleMode::Scale2x;
            break;
        case 'f':
            scale = ScaleMode::Fit;
            break;
        case OptHelp:
            PrintHelp();
            return 0;
        default:
            break;
        }
    }
    if (optind == argc)
    {
        PrintHelp();
        printf( "\n" );
        mclog( LogLevel::Error, "Image file name must be provided" );
        return 1;
    }

    const char* imageFile = argv[optind];
    std::unique_ptr<Bitmap> bitmap;
    auto imageThread = std::thread( [&bitmap, imageFile] {
        bitmap.reset( LoadImage( imageFile ) );
        if( bitmap ) mclog( LogLevel::Info, "Image loaded: %ux%u", bitmap->Width(), bitmap->Height() );
    } );

    struct winsize ws;
    ioctl( 0, TIOCGWINSZ, &ws );
    mclog( LogLevel::Info, "Terminal size: %dx%d", ws.ws_col, ws.ws_row );

    int cw, ch;
    if( !blockMode )
    {
        if( !OpenTerminal() )
        {
            mclog( LogLevel::Error, "Failed to open terminal" );
            blockMode = true;
        }
        else
        {
            const auto charSizeResp = QueryTerminal( "\033[16t" );
            if( sscanf( charSizeResp.c_str(), "\033[6;%d;%dt", &ch, &cw ) != 2 )
            {
                mclog( LogLevel::Warning, "Failed to query terminal character size" );
                blockMode = true;
            }
            else
            {
                mclog( LogLevel::Info, "Terminal char size: %dx%d", cw, ch );

                const auto kittyQuery = QueryTerminal( "\033_Gi=1,s=1,v=1,a=q,t=d,f=24;AAAA\033\\\033[c" );
                if( !kittyQuery.starts_with( "\033_Gi=1;OK\033\\" ) )
                {
                    mclog( LogLevel::Warning, "Terminal does not support kitty graphics protocol" );
                    blockMode = true;
                }
            }

            CloseTerminal();
        }
    }

    imageThread.join();
    if( !bitmap ) return 1;

    if( blockMode )
    {
        uint32_t col = ws.ws_col;
        uint32_t row = std::max<uint16_t>( 1, ws.ws_row - 1 ) * 2;

        mclog( LogLevel::Info, "Virtual pixels: %ux%u", col, row );
        AdjustBitmap( *bitmap, col, row, scale );

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
    }
    else
    {
        uint32_t col = ws.ws_col * cw;
        uint32_t row = std::max<uint16_t>( 1, ws.ws_row - 1 ) * ch;

        mclog( LogLevel::Info, "Pixels available: %ux%u", col, row );
        AdjustBitmap( *bitmap, col, row, scale );
        const auto bmpSize = bitmap->Width() * bitmap->Height() * 4;

        z_stream strm = {};
        deflateInit( &strm, Z_BEST_SPEED );
        strm.avail_in = bmpSize;
        strm.next_in = bitmap->Data();

        std::vector<uint8_t> zdata;
        zdata.resize( deflateBound( &strm, bmpSize ) );
        strm.avail_out = zdata.size();
        strm.next_out = zdata.data();

        auto res = deflate( &strm, Z_FINISH );
        CheckPanic( res == Z_STREAM_END, "Deflate failed" );
        const auto zsize = zdata.size() - strm.avail_out;
        deflateEnd( &strm );
        mclog( LogLevel::Info, "Compression %zu -> %zu", bmpSize, zsize );

        size_t b64Size = ( ( 4 * zsize / 3 ) + 3 ) & ~3;
        char* b64Data = new char[b64Size+1];
        b64Data[b64Size] = 0;
        size_t outSize;
        base64_encode( (const char*)zdata.data(), zsize, b64Data, &outSize, 0 );
        CheckPanic( outSize == b64Size, "Base64 encoding failed" );
        mclog( LogLevel::Info, "Base64 size: %zu", b64Size );

        std::string payload;
        if( b64Size <= 4096 )
        {
            payload = std::format( "\033_Gf=32,s={},v={},a=T,o=z;{}\033\\", bitmap->Width(), bitmap->Height(), b64Data );
        }
        else
        {
            auto ptr = b64Data;
            while( b64Size > 0 )
            {
                size_t chunkSize = std::min<size_t>( 4096, b64Size );
                b64Size -= chunkSize;

                if( ptr == b64Data )
                {
                    payload.append( std::format( "\033_Gf=32,s={},v={},a=T,o=z,m=1;", bitmap->Width(), bitmap->Height() ) );
                }
                else if( b64Size > 0 )
                {
                    payload.append( "\033_Gm=1;" );
                }
                else
                {
                    payload.append( "\033_Gm=0;" );
                }
                payload.append( ptr, chunkSize );
                payload.append( "\033\\" );

                ptr += chunkSize;
            }
        }

        auto sz = payload.size();
        auto ptr = payload.c_str();
        while( sz > 0 )
        {
            auto wr = write( STDOUT_FILENO, ptr, sz );
            if( wr < 0 )
            {
                mclog( LogLevel::Error, "Failed to write to terminal" );
                return 1;
            }
            sz -= wr;
            ptr += wr;
        }

        if( bitmap->Width() < col ) printf( "\n" );

        delete[] b64Data;
    }

    return 0;
}
