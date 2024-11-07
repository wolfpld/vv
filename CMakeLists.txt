cmake_minimum_required(VERSION 3.29)

option(TRACY_ENABLE "Enable Tracy" OFF)
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_CXX_STANDARD 20)

project(ModernCore
    LANGUAGES C CXX
)

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

pkg_check_modules(HEIF REQUIRED libheif)
pkg_check_modules(JPEG REQUIRED libjpeg)
pkg_check_modules(JXL REQUIRED libjxl libjxl_threads)
pkg_check_modules(PNG REQUIRED libpng)
pkg_check_modules(WEBP REQUIRED libwebp)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/src)
add_compile_definitions(DISABLE_CALLSTACK)

if(TRACY_ENABLE)
    add_definitions(-DTRACY_ENABLE)
    add_compile_options(-g -O3 -fno-omit-frame-pointer)
endif()

add_custom_target(git-ref
    COMMAND python3 ${CMAKE_CURRENT_LIST_DIR}/cmake/git-ref.py
    BYPRODUCTS GitRef.hpp
)

# mcoreutil

set(MCOREUTIL_SRC
    src/util/Bitmap.cpp
    src/util/Callstack.cpp
    src/util/Home.cpp
    src/util/Logs.cpp
    src/util/stb_image_resize_impl.cpp
)

add_library(mcoreutil ${MCOREUTIL_SRC})
target_link_libraries(mcoreutil PRIVATE
    Tracy::TracyClient
)
target_include_directories(mcoreutil PRIVATE ${stb_SOURCE_DIR})

# mcoreimage

set(MCOREIMAGE_SRC
    src/image/bcdec.c
    src/image/DdsLoader.cpp
    src/image/HeifLoader.cpp
    src/image/ImageLoader.cpp
    src/image/JpgLoader.cpp
    src/image/JxlLoader.cpp
    src/image/PngLoader.cpp
    src/image/PvrLoader.cpp
    src/image/WebpLoader.cpp
)

add_library(mcoreimage ${MCOREIMAGE_SRC})
target_link_libraries(mcoreimage PRIVATE
    mcoreutil
    Tracy::TracyClient
    ${HEIF_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${JXL_LIBRARIES}
    ${PNG_LIBRARIES}
    ${WEBP_LIBRARIES}
)
target_include_directories(mcoreimage PRIVATE
    ${HEIF_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIRS}
    ${JXL_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${WEBP_INCLUDE_DIRS}
)

# vv

set(VV_SRC
    src/tools/vv/vv.cpp
)

add_executable(vv ${VV_SRC})
target_link_libraries(vv PRIVATE
    mcoreutil
    mcoreimage
    Tracy::TracyClient
)
