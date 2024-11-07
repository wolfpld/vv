#pragma once

#include "util/FileWrapper.hpp"
#include "util/NoCopy.hpp"

class Bitmap;

class HeifLoader
{
public:
    explicit HeifLoader( FileWrapper& file );

    NoCopy( HeifLoader );

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Bitmap* Load();

private:
    FileWrapper& m_file;
    bool m_valid;
};
