#pragma once

#include <memory>

#include <OpenEXRConfig.h>

#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
class ExrStream;

namespace OPENEXR_IMF_INTERNAL_NAMESPACE { class RgbaInputFile; }

class ExrLoader
{
public:
    explicit ExrLoader( FileWrapper& file );
    ~ExrLoader();

    NoCopy( ExrLoader );

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Bitmap* Load();

private:
    std::unique_ptr<ExrStream> m_stream;
    std::unique_ptr<OPENEXR_IMF_INTERNAL_NAMESPACE::RgbaInputFile> m_exr;

    bool m_valid;
};
