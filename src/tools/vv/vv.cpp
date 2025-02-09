#include <algorithm>
#include <libbase64.h>
#include <format>
#include <getopt.h>
#include <memory>
#include <thread>
#include <sixel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>
#include <zlib.h>

#include "Terminal.hpp"
#include "image/ImageLoader.hpp"
#include "util/Bitmap.hpp"
#include "util/BitmapAnim.hpp"
#include "util/BitmapHdr.hpp"
#include "util/Callstack.hpp"
#include "util/Logs.hpp"
#include "util/Panic.hpp"
#include "util/TaskDispatch.hpp"
#include "util/Tonemapper.hpp"
#include "util/VectorImage.hpp"

namespace {
void PrintHelp()
{
    printf( "Usage: vv [options] <image>\n" );
    printf( "Options:\n" );
    printf( "  -b, --block                  Use text-only block mode\n" );
    printf( "  -6, --sixel                  Use sixel graphics mode\n" );
    printf( "  -s, --scale                  Try to scale up image to 2x\n" );
    printf( "  -f, --fit                    Fit image to terminal size\n" );
    printf( "  -G, --background [color]     Set background color to RRGGBB in hex\n" );
    printf( "  -g, --checkerboard           Use checkerboard background\n" );
    printf( "  -A, --noanim                 Disable animation\n" );
    printf( "  -w, --write [file.png]       Write output to file\n" );
    printf( "  -t, --tonemap [operator]     Choose HDR tone mapping operator\n" );
    printf( "  --help                       Print this help\n" );
    printf( "\nTone mapping operators:\n" );
    printf( "  pbr (default)\n" );
    printf( "  agx\n" );
    printf( "  agx-golden\n" );
    printf( "  agx-punchy\n" );
}

enum class ScaleMode
{
    None,
    Fit,
    Scale2x,
};

void AdjustBitmap( std::unique_ptr<Bitmap>& bitmap, std::unique_ptr<BitmapAnim>& anim, const std::unique_ptr<VectorImage>& vector, uint32_t col, uint32_t row, ScaleMode scale )
{
    if( anim )
    {
        const auto& bmp = anim->GetFrame( 0 ).bmp;
        const auto w = bmp->Width();
        const auto h = bmp->Height();

        if( scale == ScaleMode::Fit || w > col || h > row )
        {
            const auto ratio = std::min( float( col ) / w, float( row ) / h );
            const auto rw = uint32_t( w * ratio );
            const auto rh = uint32_t( h * ratio );
            anim->Resize( rw, rh );
            mclog( LogLevel::Info, "Animation resized: %ux%u", rw, rh );
        }
        else if( scale == ScaleMode::Scale2x && w * 2 <= col && h * 2 <= row )
        {
            anim->Resize( w * 2, h * 2 );
            mclog( LogLevel::Info, "Animation upscaled: %ux%u", w * 2, h * 2 );
        }
    }
    else if( bitmap )
    {
        bitmap->NormalizeOrientation();

        const auto w = bitmap->Width();
        const auto h = bitmap->Height();

        if( scale == ScaleMode::Fit || w > col || h > row )
        {
            const auto ratio = std::min( float( col ) / w, float( row ) / h );
            bitmap->Resize( w * ratio, h * ratio );
            mclog( LogLevel::Info, "Image resized: %ux%u", bitmap->Width(), bitmap->Height() );
        }
        else if( scale == ScaleMode::Scale2x && w * 2 <= col && h * 2 <= row )
        {
            bitmap->Resize( w * 2, h * 2 );
            mclog( LogLevel::Info, "Image upscaled: %ux%u", bitmap->Width(), bitmap->Height() );
        }
    }
    else
    {
        CheckPanic( vector, "No image data" );

        auto w = vector->Width();
        auto h = vector->Height();

        if( w < 0 || h < 0 )
        {
            w = col;
            h = row;
        }

        if( scale == ScaleMode::Fit || w > col || h > row )
        {
            const auto ratio = std::min( float( col ) / w, float( row ) / h );
            bitmap = vector->Rasterize( w * ratio, h * ratio );
        }
        else if( scale == ScaleMode::Scale2x && w * 2 <= col && h * 2 <= row )
        {
            bitmap = vector->Rasterize( w * 2, h * 2 );
        }
        else
        {
            bitmap = vector->Rasterize( w, h );
        }

        mclog( LogLevel::Info, "Image rasterized: %ux%u", bitmap->Width(), bitmap->Height() );
    }
}

void FillBackground( Bitmap& bitmap, uint32_t bg )
{
    const auto bgc = bg | 0xFF000000;
    const auto bgr = ( bg       ) & 0xFF;
    const auto bgg = ( bg >> 8  ) & 0xFF;
    const auto bgb = ( bg >> 16 ) & 0xFF;

    auto px = (uint32_t*)bitmap.Data();
    auto sz = bitmap.Width() * bitmap.Height();
    while( sz-- )
    {
        const auto a = *px >> 24;
        if( a == 0 )
        {
            *px = bgc;
        }
        else if( a != 255 )
        {
            const auto r = ( *px       ) & 0xFF;
            const auto g = ( *px >> 8  ) & 0xFF;
            const auto b = ( *px >> 16 ) & 0xFF;

            const auto ro = bgr + a * ( r - bgr ) / 255;
            const auto go = bgg + a * ( g - bgg ) / 255;
            const auto bo = bgb + a * ( b - bgb ) / 255;

            *px = ( bo << 16 ) | ( go << 8 ) | ro | 0xFF000000;
        }
        px++;
    }
}

void FillCheckerboard( Bitmap& bitmap )
{
    constexpr auto dist = 32;
    constexpr auto bg0 = 128 + dist;
    constexpr auto bg1 = 128 - dist;

    auto px = (uint32_t*)bitmap.Data();
    const auto bw = bitmap.Width();
    const auto bh = bitmap.Height();

    for( uint32_t h = 0; h<bh; h++ )
    {
        for( uint32_t w = 0; w<bw; w++ )
        {
            const auto a = *px >> 24;

            if( a != 255 )
            {
                const auto sel = ( w/8 + h/8 ) & 1;
                const auto bg = sel ? bg0 : bg1;

                if( a == 0 )
                {
                    *px = bg | (bg << 8 ) | (bg << 16) | 0xFF000000;
                }
                else
                {
                    const auto r = ( *px       ) & 0xFF;
                    const auto g = ( *px >> 8  ) & 0xFF;
                    const auto b = ( *px >> 16 ) & 0xFF;

                    const auto ro = bg + a * ( r - bg ) / 255;
                    const auto go = bg + a * ( g - bg ) / 255;
                    const auto bo = bg + a * ( b - bg ) / 255;

                    *px = ( bo << 16 ) | ( go << 8 ) | ro | 0xFF000000;
                }
            }

            px++;
        }
    }
}

void FillBackground( BitmapAnim& anim, uint32_t bg )
{
    for( size_t i=0; i<anim.FrameCount(); i++ )
    {
        FillBackground( *anim.GetFrame( i ).bmp, bg );
    }
}

void FillCheckerboard( BitmapAnim& anim )
{
    for( size_t i=0; i<anim.FrameCount(); i++ )
    {
        FillCheckerboard( *anim.GetFrame( i ).bmp );
    }
}

void PrintBitmapBlock( Bitmap& bitmap )
{
    auto px0 = (uint32_t*)bitmap.Data();
    auto px1 = px0 + bitmap.Width();

    for( int y=0; y<bitmap.Height() / 2; y++ )
    {
        for( int x=0; x<bitmap.Width(); x++ )
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
        px0 += bitmap.Width();
        px1 += bitmap.Width();
    }
    if( ( bitmap.Height() & 1 ) != 0 )
    {
        for( int x=0; x<bitmap.Width(); x++ )
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

bool UploadKittyImage( Bitmap& bitmap, const char* queryPart, bool anim = false )
{
    const auto bmpSize = bitmap.Width() * bitmap.Height() * 4;

    z_stream strm = {};
    deflateInit( &strm, Z_BEST_SPEED );
    strm.avail_in = bmpSize;
    strm.next_in = bitmap.Data();

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
        payload = std::format( "\033_Gf=32,s={},v={},{},o=z;{}\033\\", bitmap.Width(), bitmap.Height(), queryPart, b64Data );
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
                payload.append( std::format( "\033_Gf=32,s={},v={},{},o=z,m=1;", bitmap.Width(), bitmap.Height(), queryPart ) );
            }
            else
            {
                int m = b64Size > 0 ? 1 : 0;
                if( anim )
                {
                    payload.append( std::format( "\033_Gm={},a=f;", m ) );
                }
                else
                {
                    payload.append( std::format( "\033_Gm={};", m ) );
                }
            }

            payload.append( ptr, chunkSize );
            payload.append( "\033\\" );

            ptr += chunkSize;
        }
    }
    delete[] b64Data;

    auto sz = payload.size();
    auto ptr = payload.c_str();
    while( sz > 0 )
    {
        auto wr = write( STDOUT_FILENO, ptr, sz );
        if( wr < 0 )
        {
            mclog( LogLevel::Error, "Failed to write to terminal" );
            return false;
        }
        sz -= wr;
        ptr += wr;
    }

    return true;
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
        { "sixel", no_argument, nullptr, '6' },
        { "background", required_argument, nullptr, 'G' },
        { "checkerboard", no_argument, nullptr, 'g' },
        { "noanim", no_argument, nullptr, 'A' },
        { "write", required_argument, nullptr, 'w' },
        { "tonemap", required_argument, nullptr, 't' },
        { "help", no_argument, nullptr, OptHelp },
        {}
    };

    enum class GfxMode
    {
        Kitty,
        Sixel,
        Block,
        WriteFile
    };

    GfxMode gfxMode = GfxMode::Kitty;
    ScaleMode scale = ScaleMode::None;
    int bg = -2;
    bool disableAnimation = false;
    const char* writeFn = nullptr;
    ToneMap::Operator tonemap = ToneMap::Operator::PbrNeutral;

    int opt;
    while( ( opt = getopt_long( argc, argv, "debsf6G:gAw:t:", longOptions, nullptr ) ) != -1 )
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
            gfxMode = GfxMode::Block;
            break;
        case 's':
            scale = ScaleMode::Scale2x;
            break;
        case 'f':
            scale = ScaleMode::Fit;
            break;
        case '6':
            gfxMode = GfxMode::Sixel;
            break;
        case 'G':
            bg = strtol( optarg, nullptr, 16 );
            bg = ( bg & 0xFF ) << 16 | ( bg & 0xFF00 ) | ( bg >> 16 );
            break;
        case 'g':
            bg = -1;
            break;
        case 'A':
            disableAnimation = true;
            break;
        case 'w':
            writeFn = optarg;
            gfxMode = GfxMode::WriteFile;
            break;
        case 't':
            if( strcmp( optarg, "pbr" ) == 0 )
            {
                tonemap = ToneMap::Operator::PbrNeutral;
            }
            else if( strcmp( optarg, "agx" ) == 0 )
            {
                tonemap = ToneMap::Operator::AgX;
            }
            else if( strcmp( optarg, "agx-golden" ) == 0 )
            {
                tonemap = ToneMap::Operator::AgXGolden;
            }
            else if( strcmp( optarg, "agx-punchy" ) == 0 )
            {
                tonemap = ToneMap::Operator::AgXPunchy;
            }
            else
            {
                mclog( LogLevel::Error, "Unknown tone mapping operator" );
                return 1;
            }
            break;
        default:
            printf( "\n" );
            [[fallthrough]];
        case OptHelp:
            PrintHelp();
            return 0;
        }
    }
    if (optind == argc)
    {
        PrintHelp();
        printf( "\n" );
        mclog( LogLevel::Error, "Image file name must be provided" );
        return 1;
    }

    const auto workerThreads = std::max( 1u, std::thread::hardware_concurrency() - 1 );
    TaskDispatch td( workerThreads, "Worker" );

    const char* imageFile = argv[optind];
    std::unique_ptr<Bitmap> bitmap;
    std::unique_ptr<BitmapAnim> anim;
    std::unique_ptr<VectorImage> vectorImage;

    auto imageThread = std::thread( [&bitmap, &anim, &vectorImage, imageFile, disableAnimation, &td, tonemap] {
        mclog( LogLevel::Info, "Loading image %s", imageFile );
        auto loader = GetImageLoader( imageFile, tonemap, &td );
        if( loader )
        {
            if( !disableAnimation && loader->IsAnimated() )
            {
                anim = loader->LoadAnim();
            }
            else if( loader->IsHdr() && loader->PreferHdr() )
            {
                auto hdr = loader->LoadHdr();
                bitmap = std::make_unique<Bitmap>( hdr->Width(), hdr->Height() );

                auto src = hdr->Data();
                auto dst = bitmap->Data();
                size_t sz = hdr->Width() * hdr->Height();
                while( sz > 0 )
                {
                    const auto chunk = std::min( sz, size_t( 16 * 1024 ) );
                    td.Queue( [src, dst, chunk, tonemap] {
                        ToneMap::Process( tonemap, (uint32_t*)dst, src, chunk );
                    } );
                    src += chunk * 4;
                    dst += chunk * 4;
                    sz -= chunk;
                }
                td.Sync();
            }
            else
            {
                bitmap = loader->Load();
            }
        }
        if( anim )
        {
            mclog( LogLevel::Info, "Animated image with %zu frames", anim->FrameCount() );
            anim->NormalizeSize();
        }
        else if( bitmap )
        {
            mclog( LogLevel::Info, "Image loaded: %ux%u", bitmap->Width(), bitmap->Height() );
        }
        else
        {
            vectorImage = LoadVectorImage( imageFile );
            if( vectorImage )
            {
                mclog( LogLevel::Info, "Vector image loaded: %ix%i", vectorImage->Width(), vectorImage->Height() );
            }
        }
    } );

    struct winsize ws;
    ioctl( 0, TIOCGWINSZ, &ws );
    mclog( LogLevel::Info, "Terminal size: %dx%d", ws.ws_col, ws.ws_row );

    int cw, ch;
    if( gfxMode != GfxMode::Block && gfxMode != GfxMode::WriteFile )
    {
        if( !OpenTerminal() )
        {
            mclog( LogLevel::Error, "Failed to open terminal" );
            gfxMode = GfxMode::Block;
        }
        else
        {
            atexit( CloseTerminal );

            const auto charSizeResp = QueryTerminal( "\033[16t" );
            if( sscanf( charSizeResp.c_str(), "\033[6;%d;%dt", &ch, &cw ) != 2 )
            {
                mclog( LogLevel::Warning, "Failed to query terminal character size" );
                gfxMode = GfxMode::Block;
            }
            else
            {
                mclog( LogLevel::Info, "Terminal char size: %dx%d", cw, ch );

                const auto gfxQuery = QueryTerminal( "\033_Gi=1,s=1,v=1,a=q,t=d,f=24;AAAA\033\\\033[c" );
                if( !gfxQuery.starts_with( "\033_Gi=1;OK\033\\" ) )
                {
                    mclog( LogLevel::Info, "Terminal does not support kitty graphics protocol" );

                    // See https://invisible-island.net/xterm/ctlseqs/ctlseqs.pdf, page 12
                    if( (
                          (
                            gfxQuery.starts_with( "\033[?12;" ) ||
                            gfxQuery.starts_with( "\033[?61;" ) ||
                            gfxQuery.starts_with( "\033[?62;" ) ||
                            gfxQuery.starts_with( "\033[?63;" ) ||
                            gfxQuery.starts_with( "\033[?64;" ) ||
                            gfxQuery.starts_with( "\033[?65;" )
                          ) && (
                            gfxQuery.find( ";4;" ) != std::string::npos ||
                            gfxQuery.find( ";4c" ) != std::string::npos
                          )
                        ) || (
                          gfxQuery == "\033[?1;2;4c"    // fucking tmux can't read the specs
                        )
                      )
                    {
                        mclog( LogLevel::Info, "Fallback to sixel graphics protocol" );
                        gfxMode = GfxMode::Sixel;
                    }
                    else
                    {
                        mclog( LogLevel::Warning, "Terminal does not support sixel graphics protocol" );
                        gfxMode = GfxMode::Block;
                    }
                }
            }
        }
    }

    imageThread.join();
    if( !bitmap && !anim && !vectorImage )
    {
        mclog( LogLevel::Error, "Failed to load image %s", imageFile );
        return 1;
    }

    if( gfxMode == GfxMode::Block )
    {
        uint32_t col = ws.ws_col;
        uint32_t row = std::max<uint16_t>( 1, ws.ws_row - 1 ) * 2;

        mclog( LogLevel::Info, "Virtual pixels: %ux%u", col, row );
        AdjustBitmap( bitmap, anim, vectorImage, col, row, scale );

        if( anim )
        {
            if( bg >= 0 ) FillBackground( *anim, bg );
            else if( bg == -1 ) FillCheckerboard( *anim );
        }
        else
        {
            if( bg >= 0 ) FillBackground( *bitmap, bg );
            else if( bg == -1 ) FillCheckerboard( *bitmap );
        }

        if( anim )
        {
            printf( "\033c" );
            for(;;)
            {
                for( size_t i=0; i<anim->FrameCount(); i++ )
                {
                    printf( "\033[s" );
                    const auto& frame = anim->GetFrame( i );
                    PrintBitmapBlock( *frame.bmp );
                    usleep( frame.delay_us );
                    printf( "\033[u" );
                }
            }
        }
        else
        {
            PrintBitmapBlock( *bitmap );
        }
    }
    else if( gfxMode == GfxMode::Sixel )
    {
        if( anim )
        {
            mclog( LogLevel::Error, "Animation is not supported in sixel graphics mode" );
            return 1;
        }

        if( bg == -2 ) bg = -1;

        uint32_t col = ws.ws_col * cw;
        uint32_t row = std::max<uint16_t>( 1, ws.ws_row - 1 ) * ch;

        mclog( LogLevel::Info, "Pixels available: %ux%u", col, row );
        AdjustBitmap( bitmap, anim, vectorImage, col, row, scale );

        if( bg >= 0 ) FillBackground( *bitmap, bg );
        else if( bg == -1 ) FillCheckerboard( *bitmap );

        sixel_dither_t* dither;
        sixel_dither_new( &dither, -1, nullptr );
        sixel_dither_initialize( dither, bitmap->Data(), bitmap->Width(), bitmap->Height(), SIXEL_PIXELFORMAT_RGBA8888, SIXEL_LARGE_AUTO, SIXEL_REP_AUTO, SIXEL_QUALITY_FULL );

        sixel_output_t* output;
        sixel_output_new( &output, []( char* data, int size, void* ) -> int {
            return write( STDOUT_FILENO, data, size );
        }, nullptr, nullptr );

        sixel_encode( bitmap->Data(), bitmap->Width(), bitmap->Height(), -1, dither, output );

        sixel_output_destroy( output );
        sixel_dither_destroy( dither );
    }
    else if( gfxMode == GfxMode::Kitty )
    {
        uint32_t col = ws.ws_col * cw;
        uint32_t row = std::max<uint16_t>( 1, ws.ws_row - 1 ) * ch;

        mclog( LogLevel::Info, "Pixels available: %ux%u", col, row );
        AdjustBitmap( bitmap, anim, vectorImage, col, row, scale );

        if( anim )
        {
            if( bg >= 0 ) FillBackground( *anim, bg );
            else if( bg == -1 ) FillCheckerboard( *anim );
        }
        else
        {
            if( bg >= 0 ) FillBackground( *bitmap, bg );
            else if( bg == -1 ) FillCheckerboard( *bitmap );
        }

        if( anim )
        {
            int id = -1;
            for( size_t i=0; i<anim->FrameCount(); i++ )
            {
                const auto& frame = anim->GetFrame( i );
                const auto delay_ms = std::max<uint32_t>( frame.delay_us / 1000, 1 );
                std::string query;
                if( i == 0 )
                {
                    query = std::format( "I=1,z={}", delay_ms );
                    if( !UploadKittyImage( *anim->GetFrame( i ).bmp, query.c_str() ) ) return 1;

                    auto res = QueryTerminal();
                    if( !res.ends_with( ";OK\033\\" ) )
                    {
                        mclog( LogLevel::Error, "Failed to upload image: %s", res.c_str() + 1 );
                        return 1;
                    }

                    sscanf( res.c_str(), "\033_Gi=%i;OK\033\\", &id );
                    mclog( LogLevel::Info, "Image ID: %i", id );
                }
                else
                {
                    query = std::format( "a=f,i={},z={}", id, delay_ms );
                    if( !UploadKittyImage( *anim->GetFrame( i ).bmp, query.c_str(), true ) ) return 1;
                }
            }

            auto query = std::format( "\033_Ga=p,i={},q=1\033\\\033_Ga=a,i={},s=3,v=1,q=1\033\\", id, id );
            write( STDOUT_FILENO, query.c_str(), query.size() );

            if( anim->GetFrame( 0 ).bmp->Width() < col ) printf( "\n" );
        }
        else
        {
            if( !UploadKittyImage( *bitmap, "a=T" ) ) return 1;
            if( bitmap->Width() < col ) printf( "\n" );
        }
    }
    else if( gfxMode == GfxMode::WriteFile )
    {
        std::shared_ptr<Bitmap> img;
        if( anim )
        {
            img = anim->GetFrame( 0 ).bmp;
        }
        else if( bitmap )
        {
            img.reset( bitmap.release() );
        }
        else
        {
            CheckPanic( vectorImage, "No image data" );
            img = vectorImage->Rasterize( vectorImage->Width(), vectorImage->Height() );
        }

        if( bg >= 0 ) FillBackground( *img, bg );
        else if( bg == -1 ) FillCheckerboard( *img );

        img->SavePng( writeFn );
    }
    else
    {
        CheckPanic( false, "Invalid graphics mode" );
    }

    return 0;
}
