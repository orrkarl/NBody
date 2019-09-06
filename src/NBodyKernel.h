#pragma once

#include <string>

#include "common.h"

class NBodyKernel : public cl::Kernel
{
public:
	static inline const char* const NAME = "process_particles";

	NBodyKernel(const cl::Program& program)
		: cl::Kernel(program, NAME)
	{
	}

	NBodyKernel()
		: cl::Kernel()
	{
	}

	void setParticleBuffer(const cl::Buffer& src)
	{
		setArg(PARTICLE_SOURCE_BUFFER_INDEX, src);
	}

	void setParticleCount(const ulong_t count)
	{
		setArg(PARTICLE_COUNT_INDEX, count);
	}

	void setStepSize(const float step)
	{
		setArg(SIMULATION_STEP_SIZE_INDEX, step);
	}

	void setDestinationBuffer(const cl::Buffer& res)
	{
		setArg(PARTICLE_DESTINATION_BUFFER_INDEX, res);
	}

private:
	static inline const uint_t PARTICLE_SOURCE_BUFFER_INDEX		 = 0;
	static inline const uint_t PARTICLE_COUNT_INDEX				 = 1;
	static inline const uint_t SIMULATION_STEP_SIZE_INDEX		 = 2;
	static inline const uint_t PARTICLE_DESTINATION_BUFFER_INDEX = 3;
};
