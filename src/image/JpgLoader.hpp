#pragma once

#include "ImageLoader.hpp"
#include "util/NoCopy.hpp"

class Bitmap;

class JpgLoader : public ImageLoader
{
public:
    explicit JpgLoader( std::shared_ptr<FileWrapper> file );

    NoCopy( JpgLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;

private:
    bool m_valid;
};
