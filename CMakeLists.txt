cmake_minimum_required(VERSION 3.29)

option(TRACY_ENABLE "Enable Tracy" OFF)
set(BUILD_SHARED_LIBS OFF)
option(MARCH_NATIVE "Enable -march=native" ON)

set(CMAKE_CXX_STANDARD 20)

project(ModernCore
    LANGUAGES C CXX
)

include(CheckCompilerFlag)
include(FindPkgConfig)
include(cmake/config.cmake)
include(cmake/CPM.cmake)

if(NOT TRACY_ENABLE)
    set(TRACY_OPTIONS "TRACY_STATIC ON")
endif()

CPMAddPackage(
    NAME tracy
    GITHUB_REPOSITORY wolfpld/tracy
    GIT_TAG master
    OPTIONS ${TRACY_OPTIONS}
    EXCLUDE_FROM_ALL TRUE
)

CPMAddPackage(
    NAME stb
    GITHUB_REPOSITORY nothings/stb
    GIT_TAG 2e2bef463a5b53ddf8bb788e25da6b8506314c08
    DOWNLOAD_ONLY TRUE
)

CPMAddPackage(
    NAME base64
    GITHUB_REPOSITORY aklomp/base64
    GIT_TAG v0.5.2
    OPTIONS
        "BASE64_BUILD_CLI OFF"
        "BASE64_WITH_OpenMP OFF"
    EXCLUDE_FROM_ALL TRUE
)

pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(EXIF REQUIRED libexif)
pkg_check_modules(EXR REQUIRED OpenEXR)
pkg_check_modules(HEIF REQUIRED libheif)
pkg_check_modules(JPEG REQUIRED libjpeg)
pkg_check_modules(JXL REQUIRED libjxl libjxl_threads)
pkg_check_modules(LCMS REQUIRED lcms2)
pkg_check_modules(LZ4 REQUIRED liblz4)
pkg_check_modules(PNG REQUIRED libpng)
pkg_check_modules(PUGIXML REQUIRED pugixml)
pkg_check_modules(RAW REQUIRED libraw)
pkg_check_modules(RSVG REQUIRED librsvg-2.0)
pkg_check_modules(SIXEL REQUIRED libsixel)
pkg_check_modules(TIFF REQUIRED libtiff-4)
pkg_check_modules(WEBP REQUIRED libwebpdemux)
pkg_check_modules(ZLIB REQUIRED zlib)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/src)
add_compile_definitions(DISABLE_CALLSTACK)

if(TRACY_ENABLE)
    add_definitions(-DTRACY_ENABLE)
    add_compile_options(-g -O3 -fno-omit-frame-pointer)
endif()

if(MARCH_NATIVE)
    check_compiler_flag(CXX "-march=native" HAS_MARCH_NATIVE)
    if(HAS_MARCH_NATIVE)
        add_compile_options(-march=native)
    endif()
endif()

# embed

add_executable(embed helpers/embed.cpp)
target_link_libraries(embed PRIVATE ${LZ4_LINK_LIBRARIES})
target_include_directories(embed PRIVATE ${LZ4_INCLUDE_DIRS})

function(Embed LIST NAME FILE)
    add_custom_command(
        OUTPUT data/${NAME}.cpp data/${NAME}.hpp
        COMMAND embed ${NAME} ${CMAKE_CURRENT_LIST_DIR}/${FILE} data/${NAME}
        DEPENDS embed ${CMAKE_CURRENT_LIST_DIR}/${FILE}
    )
    list(APPEND ${LIST} data/${NAME}.cpp)
    return(PROPAGATE ${LIST})
endfunction()

# mcoreutil

add_custom_command(
    OUTPUT CmykIcm.cpp CmykIcm.hpp
    COMMAND embed CmykIcm ${CMAKE_CURRENT_LIST_DIR}/src/util/cmyk.icm CmykIcm
    DEPENDS embed ${CMAKE_CURRENT_LIST_DIR}/src/util/cmyk.icm
)

set(MCOREUTIL_SRC
    src/util/Bitmap.cpp
    src/util/BitmapAnim.cpp
    src/util/BitmapHdr.cpp
    src/util/Callstack.cpp
    src/util/EmbedData.cpp
    src/util/FileBuffer.cpp
    src/util/Home.cpp
    src/util/Logs.cpp
    src/util/TaskDispatch.cpp
    src/util/Tonemapper.cpp
    src/util/TonemapperAgx.cpp
    src/util/TonemapperPbr.cpp
    src/util/stb_image_resize_impl.cpp
)

Embed(MCOREUTIL_SRC CmykIcm src/util/cmyk.icm)

add_library(mcoreutil ${MCOREUTIL_SRC})
target_link_libraries(mcoreutil PRIVATE
    Tracy::TracyClient
    ${LZ4_LINK_LIBRARIES}
    ${PNG_LINK_LIBRARIES}
)
target_include_directories(mcoreutil PRIVATE
    ${LZ4_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${stb_SOURCE_DIR}
)

# mcoreimage

set(MCOREIMAGE_SRC
    src/image/bcdec.c
    src/image/DdsLoader.cpp
    src/image/ExrLoader.cpp
    src/image/HeifLoader.cpp
    src/image/ImageLoader.cpp
    src/image/JpgLoader.cpp
    src/image/JxlLoader.cpp
    src/image/PcxLoader.cpp
    src/image/PngLoader.cpp
    src/image/PvrLoader.cpp
    src/image/RawLoader.cpp
    src/image/StbImageLoader.cpp
    src/image/TiffLoader.cpp
    src/image/WebpLoader.cpp
    src/image/vector/PdfImage.cpp
    src/image/vector/SvgImage.cpp
)

add_library(mcoreimage ${MCOREIMAGE_SRC})
target_link_libraries(mcoreimage PRIVATE
    mcoreutil
    Tracy::TracyClient
    ${CAIRO_LINK_LIBRARIES}
    ${EXIF_LINK_LIBRARIES}
    ${EXR_LINK_LIBRARIES}
    ${HEIF_LINK_LIBRARIES}
    ${JPEG_LINK_LIBRARIES}
    ${JXL_LINK_LIBRARIES}
    ${LCMS_LINK_LIBRARIES}
    ${PNG_LINK_LIBRARIES}
    ${PUGIXML_LINK_LIBRARIES}
    ${RAW_LINK_LIBRARIES}
    ${RSVG_LINK_LIBRARIES}
    ${TIFF_LINK_LIBRARIES}
    ${WEBP_LINK_LIBRARIES}
)
target_include_directories(mcoreimage PRIVATE
    ${CAIRO_INCLUDE_DIRS}
    ${EXIF_INCLUDE_DIRS}
    ${EXR_INCLUDE_DIRS}
    ${HEIF_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIRS}
    ${JXL_INCLUDE_DIRS}
    ${LCMS_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${PUGIXML_INCLUDE_DIRS}
    ${RAW_INCLUDE_DIRS}
    ${RSVG_INCLUDE_DIRS}
    ${TIFF_INCLUDE_DIRS}
    ${WEBP_INCLUDE_DIRS}
    ${stb_SOURCE_DIR}
)

# vv

set(VV_SRC
    src/tools/vv/vv.cpp
    src/tools/vv/Terminal.cpp
)

add_executable(vv ${VV_SRC})
target_include_directories(vv PRIVATE
    ${SIXEL_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
)
target_link_libraries(vv PRIVATE
    mcoreutil
    mcoreimage
    Tracy::TracyClient
    base64
    ${SIXEL_LINK_LIBRARIES}
    ${ZLIB_LINK_LIBRARIES}
)
