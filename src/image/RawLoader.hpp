#pragma once

#include <memory>

#include "ImageLoader.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
class FileBuffer;
class LibRaw;

class RawLoader : public ImageLoader
{
public:
    explicit RawLoader( std::shared_ptr<FileWrapper> file );
    ~RawLoader() override;

    NoCopy( RawLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;

private:
    std::unique_ptr<LibRaw> m_raw;
    std::unique_ptr<FileBuffer> m_buf;

    bool m_valid;
};
