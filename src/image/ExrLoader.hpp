#pragma once

#include <memory>

#include <OpenEXRConfig.h>

#include "ImageLoader.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
class ExrStream;
class FileWrapper;
class TaskDispatch;

namespace OPENEXR_IMF_INTERNAL_NAMESPACE { class RgbaInputFile; }

class ExrLoader : public ImageLoader
{
public:
    explicit ExrLoader( std::shared_ptr<FileWrapper> file, ToneMap::Operator tonemap, TaskDispatch* td );
    ~ExrLoader() override;

    NoCopy( ExrLoader );

    [[nodiscard]] bool IsValid() const override;
    [[nodiscard]] bool IsHdr() override { return true; }

    [[nodiscard]] std::unique_ptr<Bitmap> Load() override;
    [[nodiscard]] std::unique_ptr<BitmapHdr> LoadHdr() override;

private:
    std::unique_ptr<ExrStream> m_stream;
    std::unique_ptr<OPENEXR_IMF_INTERNAL_NAMESPACE::RgbaInputFile> m_exr;

    TaskDispatch* m_td;

    bool m_valid;
    ToneMap::Operator m_tonemap;
};
