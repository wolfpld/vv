#pragma once

#include <memory>

#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
class ExrStream;

namespace Imf_3_2 { class RgbaInputFile; }

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
    std::unique_ptr<Imf_3_2::RgbaInputFile> m_exr;

    bool m_valid;
};
