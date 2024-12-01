#pragma once

#include "ImageLoader.hpp"
#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;

class HeifLoader : public ImageLoader
{
public:
    explicit HeifLoader( std::shared_ptr<FileWrapper> file );

    NoCopy( HeifLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;

private:
    bool m_valid;
};
