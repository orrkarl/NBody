#pragma once

#include "cl.h"

void prepareInterop(const cl::Platform& platform, const cl::Device& dev, cl_context_properties properties[7]);
