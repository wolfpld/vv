#pragma once

#include "util/NoCopy.hpp"
#include "util/FileWrapper.hpp"

class Bitmap;

class StbImageLoader
{
public:
    explicit StbImageLoader( FileWrapper& file );

    NoCopy( StbImageLoader );

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Bitmap* Load();

private:
    FileWrapper& m_file;
    bool m_valid;
};
