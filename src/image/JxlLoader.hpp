#pragma once

#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;

class JxlLoader
{
public:
    explicit JxlLoader( FileWrapper& file );

    NoCopy( JxlLoader );

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Bitmap* Load();

private:
    FileWrapper& m_file;
    bool m_valid;
};
