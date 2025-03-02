#pragma once

#include <memory>
#include <stdint.h>

#include "ImageLoader.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
class FileBuffer;
class FileWrapper;

typedef struct WebPAnimDecoder WebPAnimDecoder;

class WebpLoader : public ImageLoader
{
public:
    explicit WebpLoader( std::shared_ptr<FileWrapper> file );
    ~WebpLoader() override;

    NoCopy( WebpLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] bool IsAnimated() override;

    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;
    [[nodiscard]] std::unique_ptr<BitmapAnim> LoadAnim() override;

private:
    bool Open();

    bool m_valid;

    std::shared_ptr<FileWrapper> m_file;
    std::unique_ptr<FileBuffer> m_buf;
    WebPAnimDecoder* m_dec;
};
