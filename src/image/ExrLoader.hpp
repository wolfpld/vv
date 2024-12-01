#pragma once

#include <memory>

#include <OpenEXRConfig.h>

#include "ImageLoader.hpp"
#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
class ExrStream;

namespace OPENEXR_IMF_INTERNAL_NAMESPACE { class RgbaInputFile; }

class ExrLoader : public ImageLoader
{
public:
    explicit ExrLoader( std::shared_ptr<FileWrapper> file );
    ~ExrLoader() override;

    NoCopy( ExrLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;

private:
    std::unique_ptr<ExrStream> m_stream;
    std::unique_ptr<OPENEXR_IMF_INTERNAL_NAMESPACE::RgbaInputFile> m_exr;

    bool m_valid;
};
