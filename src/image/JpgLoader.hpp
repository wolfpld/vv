#pragma once

#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;

class JpgLoader
{
public:
    explicit JpgLoader( FileWrapper& file );

    NoCopy( JpgLoader );

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Bitmap* Load();

private:
    FileWrapper& m_file;
    bool m_valid;
};
