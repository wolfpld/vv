#pragma once

#include "util/NoCopy.hpp"
#include "util/FileWrapper.hpp"

class Bitmap;

class PcxLoader
{
public:
    explicit PcxLoader( FileWrapper& file );

    NoCopy( PcxLoader );

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Bitmap* Load();

private:
    FileWrapper& m_file;
    bool m_valid;
};
