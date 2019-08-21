#pragma once

#include <string>

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

class NBodyKernel : cl::Kernel
{
public:
	static const std::string NAME;

	NBodyKernel(const cl::Program& program);

	void setParticleBuffer();

	void setParticleCount();

	void setStepSize();

	void setDestinationBuffer();
};
