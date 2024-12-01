#pragma once

#include <memory>

#include "util/FileWrapper.hpp"
#include "util/VectorImage.hpp"

class FileBuffer;

typedef struct _GInputStream GInputStream;
typedef struct _RsvgHandle RsvgHandle;
typedef struct _GFile GFile;

class SvgImage : public VectorImage
{
public:
    explicit SvgImage( FileWrapper& file, const char* path );
    ~SvgImage() override;

    NoCopy( SvgImage );

    [[nodiscard]] bool IsValid() const override;

    [[nodiscard]] int Width() const override { return m_width; }
    [[nodiscard]] int Height() const override { return m_height; }

    [[nodiscard]] std::unique_ptr<Bitmap> Rasterize( int width, int height ) override;

private:
    std::unique_ptr<FileBuffer> m_buf;

    GFile* m_file;
    GInputStream* m_stream;
    RsvgHandle* m_handle;

    int m_width = -1;
    int m_height = -1;
};
