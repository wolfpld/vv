#include "BitmapAnim.hpp"
#include "Logs.hpp"

BitmapAnim::BitmapAnim( uint32_t frameCount )
{
    m_frames.reserve( frameCount );
}

void BitmapAnim::AddFrame( std::shared_ptr<Bitmap> bmp, uint32_t delay_us )
{
    m_frames.push_back( { std::move( bmp ), delay_us } );
}

void BitmapAnim::Resize( uint32_t width, uint32_t height )
{
    for( auto& frame : m_frames )
    {
        frame.bmp->Resize( width, height );
    }
}

void BitmapAnim::NormalizeSize()
{
    bool normalize = false;
    uint32_t mw = m_frames[0].bmp->Width();
    uint32_t mh = m_frames[0].bmp->Height();

    for( size_t i=1; i<m_frames.size(); i++ )
    {
        const auto fw = m_frames[i].bmp->Width();
        const auto fh = m_frames[i].bmp->Height();
        if( fw != mw || fh != mh )
        {
            normalize = true;
            if( fw > mw ) mw = fw;
            if( fh > mh ) mh = fh;
        }
    }

    mclog( LogLevel::Info, "Animation size: %ux%u", mw, mh );

    if( normalize )
    {
        for( size_t i=0; i<m_frames.size(); i++ )
        {
            const auto fw = m_frames[i].bmp->Width();
            const auto fh = m_frames[i].bmp->Height();
            if( fw != mw || fh != mh )
            {
                mclog( LogLevel::Info, "Extending frame %zu (size %ux%u)", i, fw, fh );
                m_frames[i].bmp->Extend( mw, mh );
            }
        }
    }
}
