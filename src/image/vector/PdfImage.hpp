#pragma once

#include "util/FileWrapper.hpp"
#include "util/VectorImage.hpp"

class PdfImage : public VectorImage
{
public:
    explicit PdfImage( FileWrapper& file, const char* path );
    ~PdfImage() override;

    NoCopy( PdfImage );

    [[nodiscard]] bool IsValid() const override;

    [[nodiscard]] int Width() const override { return m_width; }
    [[nodiscard]] int Height() const override { return m_height; }

    [[nodiscard]] std::unique_ptr<Bitmap> Rasterize( int width, int height ) override;

private:
    void* m_pdf;
    void* m_page;

    int m_width = -1;
    int m_height = -1;
};
