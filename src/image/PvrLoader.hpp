#pragma once

#include <stdint.h>

#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;

class PvrLoader
{
public:
    explicit PvrLoader( FileWrapper& file );

    NoCopy( PvrLoader );

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Bitmap* Load();

private:
    FileWrapper& m_file;
    bool m_valid;

    uint32_t m_format;
};
