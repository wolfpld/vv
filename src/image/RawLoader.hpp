#pragma once

#include <memory>

#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
class FileBuffer;
class LibRaw;

class RawLoader
{
public:
    explicit RawLoader( FileWrapper& file );
    ~RawLoader();

    NoCopy( RawLoader );

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Bitmap* Load();

private:
    std::unique_ptr<LibRaw> m_raw;
    std::unique_ptr<FileBuffer> m_buf;

    bool m_valid;
};
