#pragma once

#include <memory>

#include <ImfRgbaFile.h>

#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;
class ExrStream;

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
    std::unique_ptr<Imf::RgbaInputFile> m_exr;

    bool m_valid;
};
