#pragma once

#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;

class WebpLoader
{
public:
    explicit WebpLoader( FileWrapper& file );

    NoCopy( WebpLoader );

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Bitmap* Load();

private:
    FileWrapper& m_file;
    bool m_valid;
};
