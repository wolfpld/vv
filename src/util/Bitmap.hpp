#pragma once

#include <stdint.h>

class Bitmap
{
public:
    Bitmap( uint32_t width, uint32_t height );
    ~Bitmap();

    Bitmap( const Bitmap& ) = delete;
    Bitmap( Bitmap&& other ) noexcept;
    Bitmap& operator=( const Bitmap& ) = delete;
    Bitmap& operator=( Bitmap&& other ) noexcept;

    void Resize( uint32_t width, uint32_t height );
    void Extend( uint32_t width, uint32_t height );
    void FlipVertical();
    void SetAlpha( uint8_t alpha );

    [[nodiscard]] uint32_t Width() const { return m_width; }
    [[nodiscard]] uint32_t Height() const { return m_height; }
    [[nodiscard]] uint8_t* Data() { return m_data; }
    [[nodiscard]] const uint8_t* Data() const { return m_data; }

    void SavePng( const char* path ) const;

private:
    uint32_t m_width;
    uint32_t m_height;
    uint8_t* m_data;
};
