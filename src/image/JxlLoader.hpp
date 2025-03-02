#pragma once

#include <jxl/codestream_header.h>
#include <memory>
#include <vector>

#include "ImageLoader.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
class BitmapHdr;
class FileBuffer;
class FileWrapper;
typedef struct JxlDecoderStruct JxlDecoder;
typedef void* cmsHPROFILE;
typedef void* cmsHTRANSFORM;

class JxlLoader : public ImageLoader
{
public:
    struct CmsData
    {
        std::vector<float*> srcBuf;
        std::vector<float*> dstBuf;

        cmsHPROFILE profileIn;
        cmsHPROFILE profileOut;
        cmsHTRANSFORM transform;
    };

    explicit JxlLoader( std::shared_ptr<FileWrapper> file );
    ~JxlLoader() override;

    NoCopy( JxlLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] bool IsHdr() override;
    [[nodiscard]] bool PreferHdr() override;

    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;
    [[nodiscard]] std::unique_ptr<BitmapHdr> LoadHdr() override;

private:
    bool Open();

    bool m_valid;
    std::shared_ptr<FileWrapper> m_file;
    std::unique_ptr<FileBuffer> m_buf;

    void* m_runner;
    JxlDecoder* m_dec;
    JxlBasicInfo m_info;
    CmsData m_cms;
};
