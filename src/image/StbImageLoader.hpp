#pragma once

#include "ImageLoader.hpp"
#include "util/NoCopy.hpp"
#include "util/FileWrapper.hpp"

class Bitmap;

class StbImageLoader : public ImageLoader
{
public:
    explicit StbImageLoader( std::shared_ptr<FileWrapper> file );

    NoCopy( StbImageLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;

private:
    bool m_valid;
};
