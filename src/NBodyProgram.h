#pragma once

#include "cl.h"

class NBodyProgram : public cl::Program
{
public:
    NBodyProgram(const cl::Context& ctx);
};