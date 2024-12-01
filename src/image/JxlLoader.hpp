#pragma once

#include "ImageLoader.hpp"
#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;

class JxlLoader : public ImageLoader
{
public:
    explicit JxlLoader( std::shared_ptr<FileWrapper> file );

    NoCopy( JxlLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;

private:
    bool m_valid;
};
