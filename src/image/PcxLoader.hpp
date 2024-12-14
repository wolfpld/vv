#pragma once

#include "ImageLoader.hpp"
#include "util/NoCopy.hpp"

class Bitmap;

class PcxLoader : public ImageLoader
{
public:
    explicit PcxLoader( std::shared_ptr<FileWrapper> file );

    NoCopy( PcxLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;

private:
    bool m_valid;
};
