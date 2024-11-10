#pragma once

#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
struct tiff;

class TiffLoader
{
public:
    explicit TiffLoader( FileWrapper& file );
    ~TiffLoader();

    NoCopy( TiffLoader );

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Bitmap* Load();

private:
    struct tiff* m_tiff;
};
