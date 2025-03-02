#pragma once

#include <memory>
#include <stdint.h>

#include "util/FileWrapper.hpp"
#include "util/VectorImage.hpp"

class DataBuffer;

typedef struct _GInputStream GInputStream;
typedef struct _RsvgHandle RsvgHandle;

class SvgImage : public VectorImage
{
public:
    explicit SvgImage( FileWrapper& file );
    explicit SvgImage( std::shared_ptr<DataBuffer> buf );
    ~SvgImage() override;

    NoCopy( SvgImage );

    [[nodiscard]] bool IsValid() const override;

    [[nodiscard]] int Width() const override { return m_width; }
    [[nodiscard]] int Height() const override { return m_height; }

    [[nodiscard]] std::unique_ptr<Bitmap> Rasterize( int width, int height ) const override;

    void SetBorder( uint32_t border ) { m_border = border; }

private:
    std::shared_ptr<DataBuffer> m_buf;

    GInputStream* m_stream;
    RsvgHandle* m_handle;

    int m_width = -1;
    int m_height = -1;

    uint32_t m_border = 0;
};
