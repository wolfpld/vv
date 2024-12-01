#pragma once

#include "ImageLoader.hpp"
#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
struct tiff;

class TiffLoader : public ImageLoader
{
public:
    explicit TiffLoader( std::shared_ptr<FileWrapper> file );
    ~TiffLoader() override;

    NoCopy( TiffLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;

private:
    struct tiff* m_tiff;
};
