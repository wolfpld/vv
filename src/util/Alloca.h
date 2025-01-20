#pragma once

#if defined(__FreeBSD__) || defined(__NetBSD__)
#  include <stdlib.h>
#else
#  include <alloca.h>
#endif
