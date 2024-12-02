#pragma once

#include <memory>

#include "util/NoCopy.hpp"

class Bitmap;
class BitmapAnim;
class BitmapHdr;
class FileWrapper;
class VectorImage;

class ImageLoader
{
public:
    ImageLoader( std::shared_ptr<FileWrapper>&& file ) : m_file( std::move( file ) ) {}
    virtual ~ImageLoader() = default;

    NoCopy( ImageLoader );

    [[nodiscard]] virtual bool IsValid() const = 0;
    [[nodiscard]] virtual bool IsAnimated() { return false; }
    [[nodiscard]] virtual bool IsHdr() { return false; }

    [[nodiscard]] virtual std::unique_ptr<Bitmap> Load() = 0;
    [[nodiscard]] virtual std::unique_ptr<BitmapAnim> LoadAnim() { return nullptr; }
    [[nodiscard]] virtual std::unique_ptr<BitmapHdr> LoadHdr() { return nullptr; }

protected:
    std::shared_ptr<FileWrapper> m_file;
};

std::unique_ptr<ImageLoader> GetImageLoader( const char* filename );
std::unique_ptr<Bitmap> LoadImage( const char* filename );
std::unique_ptr<VectorImage> LoadVectorImage( const char* filename );
