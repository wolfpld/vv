#pragma once

#include <memory>

#include "ImageLoader.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
class BitmapHdr;
class FileBuffer;
class TaskDispatch;

struct heif_context;
struct heif_image;
struct heif_image_handle;
struct heif_color_profile_nclx;

class HeifLoader : public ImageLoader
{
    enum class Conversion
    {
        GBR,
        BT601,
        BT709,
        BT2020
    };

public:
    explicit HeifLoader( std::shared_ptr<FileWrapper> file, TaskDispatch* td );
    ~HeifLoader() override;

    NoCopy( HeifLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] bool IsHdr() override;

    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;
    [[nodiscard]] std::unique_ptr<BitmapHdr> LoadHdr() override;

private:
    bool Open();

    bool SetupDecode( bool hdr );

    [[nodiscard]] std::unique_ptr<Bitmap> LoadNoProfile();
    void LoadYCbCr( float* ptr, size_t sz, size_t offset );

    void ConvertYCbCrToRGB( float* ptr, size_t sz );
    void ApplyTransfer( float* ptr, size_t sz );

    bool m_valid;
    std::unique_ptr<FileBuffer> m_buf;

    heif_context* m_ctx;
    heif_image_handle* m_handle;
    heif_image* m_image;
    heif_color_profile_nclx* m_nclx;

    int m_width, m_height;
    int m_stride, m_bpp;
    float m_bppDiv;

    Conversion m_matrix;

    size_t m_iccSize;
    char* m_iccData;

    const uint8_t* m_planeY;
    const uint8_t* m_planeCb;
    const uint8_t* m_planeCr;
    const uint8_t* m_planeA;

    void* m_profileIn;
    void* m_profileOut;
    void* m_transform;

    TaskDispatch* m_td;
};
