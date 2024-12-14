#pragma once

#include "ImageLoader.hpp"
#include "util/NoCopy.hpp"

class Bitmap;

class PngLoader : public ImageLoader
{
public:
    explicit PngLoader( std::shared_ptr<FileWrapper> file );

    NoCopy( PngLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;

private:
    bool m_valid;
};
