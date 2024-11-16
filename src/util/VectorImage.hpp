#pragma once

#include "NoCopy.hpp"

class Bitmap;

class VectorImage
{
public:
    VectorImage() = default;
    virtual ~VectorImage() = default;
    NoCopy( VectorImage );

    [[nodiscard]] virtual bool IsValid() const = 0;

    [[nodiscard]] virtual int Width() const { return -1; }
    [[nodiscard]] virtual int Height() const { return -1; }

    [[nodiscard]] virtual Bitmap* Rasterize( int width, int height ) = 0;
};
