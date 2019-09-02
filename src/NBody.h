#pragma once

#include "common.h"

#include "DoubleBuffer.h"
#include "NBodyKernel.h"
#include "Particle.h"

class NBody
{
public:
	static void initialize();
	static void destroy() noexcept;

	NBody(const ulong_t particleCount, const float stepSize, const uint_t width = 640, const uint_t height = 480, const char* name = "NBody Simulation");

	~NBody() noexcept;

	void run();

private:
	void initGL(const uint_t particleCount, const uint_t width, const uint_t height, const char* name);
	void initCL(const ulong_t particleCount, const float stepSize);
	void periodic();

	cl::CommandQueue			m_commandQueue;
	cl::Context					m_context;
	cl::Device					m_device;
	DoubleBuffer<cl::Buffer>	m_particlesProcessingBuffer;
	NBodyKernel					m_particleProcessor;
	DoubleBuffer<GLuint>		m_particlesDrawBuffer;
	GLuint						m_glProgram;
	GLuint						m_vao;
	GLFWwindow*					m_window;
};

class NBodyInitializeGuard
{
public:
	NBodyInitializeGuard()
	{
		NBody::initialize();
	}

	~NBodyInitializeGuard() noexcept
	{
		NBody::destroy();
	}
};

