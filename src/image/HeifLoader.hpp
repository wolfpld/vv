#pragma once

#include <memory>

#include "ImageLoader.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
class BitmapHdr;
class FileBuffer;
struct heif_context;
struct heif_image_handle;
struct heif_color_profile_nclx;

class HeifLoader : public ImageLoader
{
public:
    explicit HeifLoader( std::shared_ptr<FileWrapper> file );
    ~HeifLoader() override;

    NoCopy( HeifLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] bool IsHdr() override;

    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;
    [[nodiscard]] std::unique_ptr<BitmapHdr> LoadHdr() override;

private:
    bool Open();

    [[nodiscard]] std::unique_ptr<Bitmap> LoadNoProfile();
    [[nodiscard]] std::unique_ptr<Bitmap> LoadWithIccProfile();

    [[nodiscard]] std::unique_ptr<BitmapHdr> LoadHdrNoProfile();
    [[nodiscard]] std::unique_ptr<BitmapHdr> LoadHdrWithIccProfile();

    [[nodiscard]] std::unique_ptr<BitmapHdr> LoadYCbCr();

    void ConvertYCbCrToRGB( const std::unique_ptr<BitmapHdr>& bmp );
    void ApplyTransfer( const std::unique_ptr<BitmapHdr>& bmp );

    bool m_valid;
    std::unique_ptr<FileBuffer> m_buf;

    heif_context* m_ctx;
    heif_image_handle* m_handle;
    heif_color_profile_nclx* m_nclx;

    int m_width, m_height;

    size_t m_iccSize;
    char* m_iccData;
};
