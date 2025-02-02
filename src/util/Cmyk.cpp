#include <cmyk.icm.cpp>

#include "util/Cmyk.hpp"

namespace CMYK
{

size_t ProfileSize()
{
    return cmyk_icm_size;
}

const void* ProfileData()
{
    return cmyk_icm_data;
}

}
