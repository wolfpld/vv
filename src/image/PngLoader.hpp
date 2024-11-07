#pragma once

#include "util/NoCopy.hpp"
#include "util/FileWrapper.hpp"

class Bitmap;

class PngLoader
{
public:
    explicit PngLoader( FileWrapper& file );

    NoCopy( PngLoader );

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Bitmap* Load();

private:
    FileWrapper& m_file;
    bool m_valid;
};
