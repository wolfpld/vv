> [!IMPORTANT]
> This repository will not be updated anymore. Please go to <https://github.com/wolfpld/moderncore> to follow further development of vv.

<div align="center">

# vv - terminal image viewer

![Screenshot](doc/img1.png)

</div>

With vv you can display image files directly in your terminal. This works both locally and over remote connections. An extensive range of modern image formats is supported. Image data is displayed in full color, without any color space reduction or dithering. Images are scaled to fit the available space in the terminal. Small images can be upscaled.

## Image formats

The following types of image files can be viewed in vv:

- BC (Block Compression, also known as DXTC, S3TC), in DDS container,
- OpenEXR,
- HEIF (High Efficiency Image File Format),
- AVIF (AV1 Image File Format),
- JPEG,
- JPEG XL,
- PCX,
- PNG,
- ETC (Ericsson Texture Compression), in PVR container,
- RAW, digital camera negatives, virtually all formats,
- TGA,
- BMP,
- PSD (Photoshop),
- GIF (animations not supported),
- RGBE (Radiance HDR),
- PIC (Softimage),
- PPM and PGM (only binary),
- TIFF,
- WebP (with animations),
- PDF,
- SVG.

### High dynamic range

Loading of HDR images is supported for OpenEXR, HEIF, AVIF, JPEG XL, and RGBE formats. HDR images are properly tone mapped using the Khronos PBR Neutral operator for display on the SDR terminal.

<div align="center">

![HDR image](doc/img2.png)

</div>

### Color management

Color profiles embedded in images are taken into account to ensure that images look exactly as they should.

<div align="center">

![Proper color management](doc/img6.png)

</div>

### Transparency

Images with an alpha channel are rendered with transparency over the terminal background color. If this is not suitable, vv gives you the option to render the image with either a checkerboard background or a solid color backdrop of your choice.

<div align="center">

![Transparent image](doc/img4.png)

</div>

### Vector images

Vector image formats, such as SVG, are rendered at a terminal-native resolution, with full transparency support.

<div align="center">

![Vector image](doc/img5.png)

</div>

### PDF support

For viewing PDF files, you must have the Poppler library installed on your system. This library is not used at all at build time, and is not an explicit runtime dependency of vv, unlike the other libraries.

## Terminal support

In order to be able to view images with vv, you need to use a terminal that implements the [Kitty graphics protocol](https://sw.kovidgoyal.net/kitty/graphics-protocol/). If your terminal can't do this, vv will work in a text-only fallback mode with greatly reduced image resolution.

<div align="center">

![Text mode](doc/img3.png)

</div>

Certain terminal features, such as Unicode fonts or true color support, are assumed to be always available.

On some terminals (e.g. Konsole), images may appear pixelated when using a high DPI monitor. This problem is caused by implementation details of the terminal itself, and cannot be fixed by vv. Try a different terminal if this bothers you.

### Animation

Animated images in WebP format can be played. The text-only fallback mode is driven by vv outputting images on its own. The Kitty graphics protocol allows much more sophisticated control of the animation, in which case the image will continue to play even after vv exits. Note that support for Kitty animations in terminal implementations is currently very limited.

## Building

Follow the standard CMake build process. It will create the `build/vv` executable.

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

By default vv is built with the `-march=native` compiler option, to enable SIMD processing. If you want to build an executable that will work on any machine you may want to turn this off with the `MARCH_NATIVE` CMake option.