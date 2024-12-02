#pragma once

#include <stdint.h>
#include <memory>
#include <vector>

#include "Bitmap.hpp"
#include "NoCopy.hpp"

class BitmapAnim
{
public:
    struct Frame
    {
        std::shared_ptr<Bitmap> bmp;
        uint32_t delay_us;
    };

    BitmapAnim( uint32_t frameCount );
    NoCopy( BitmapAnim );

    void AddFrame( std::shared_ptr<Bitmap> bmp, uint32_t delay_us );
    void Resize( uint32_t width, uint32_t height );
    void NormalizeSize();

    [[nodiscard]] size_t FrameCount() const { return m_frames.size(); }

    [[nodiscard]] Frame& GetFrame( size_t idx ) { return m_frames[idx]; }
    [[nodiscard]] const Frame& GetFrame( size_t idx ) const { return m_frames[idx]; }

private:
    std::vector<Frame> m_frames;
};
