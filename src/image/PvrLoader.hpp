#pragma once

#include <memory>
#include <stdint.h>

#include "ImageLoader.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
class FileWrapper;

class PvrLoader : public ImageLoader
{
public:
    explicit PvrLoader( std::shared_ptr<FileWrapper> file );

    NoCopy( PvrLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;

private:
    bool m_valid;
    std::shared_ptr<FileWrapper> m_file;

    uint32_t m_format;
};
