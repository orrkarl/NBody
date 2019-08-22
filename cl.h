#pragma once

#define __CL_ENABLE_EXCEPTIONS

#ifdef __APPLE__ || MACOSX
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif 