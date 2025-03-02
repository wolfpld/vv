#pragma once

#include <memory>

#include "util/Tonemapper.hpp"

class Bitmap;
class BitmapAnim;
class BitmapHdr;
class TaskDispatch;
class VectorImage;

class ImageLoader
{
public:
    virtual ~ImageLoader() = default;

    [[nodiscard]] virtual bool IsValid() const = 0;
    [[nodiscard]] virtual bool IsAnimated() { return false; }
    [[nodiscard]] virtual bool IsHdr() { return false; }
    [[nodiscard]] virtual bool PreferHdr() { return false; }

    [[nodiscard]] virtual std::unique_ptr<Bitmap> Load() = 0;
    [[nodiscard]] virtual std::unique_ptr<BitmapAnim> LoadAnim();
    [[nodiscard]] virtual std::unique_ptr<BitmapHdr> LoadHdr();
};

std::unique_ptr<ImageLoader> GetImageLoader( const char* filename, ToneMap::Operator tonemap, TaskDispatch* td = nullptr );
std::unique_ptr<Bitmap> LoadImage( const char* filename );
std::unique_ptr<VectorImage> LoadVectorImage( const char* filename );
