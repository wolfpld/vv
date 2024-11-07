#pragma once

#include <stdint.h>

#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;

class DdsLoader
{
public:
    explicit DdsLoader( FileWrapper& file );

    NoCopy( DdsLoader );

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Bitmap* Load();

private:
    FileWrapper& m_file;
    bool m_valid;

    uint32_t m_format;
    uint32_t m_offset;
};
