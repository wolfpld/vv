#pragma once

#include "ImageLoader.hpp"
#include "util/NoCopy.hpp"

class Bitmap;

class StbImageLoader : public ImageLoader
{
public:
    explicit StbImageLoader( std::shared_ptr<FileWrapper> file );

    NoCopy( StbImageLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] bool IsHdr() override;

    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;
    [[nodiscard]] std::unique_ptr<BitmapHdr> LoadHdr() override;

private:
    bool m_valid;
    bool m_hdr;
};
