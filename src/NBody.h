#pragma once

#include "common.h"

#include "DoubleBuffer.h"
#include "NBodyKernel.h"
#include "Particle.h"

#include <memory>

class NBody
{
public:
	static void initialize();
	static void destroy() noexcept;

	NBody(const ulong_t particleCount, const float stepSize, const uint_t width = 640, const uint_t height = 480, const char* name = "NBody Simulation");

	~NBody() noexcept;

	void run();

private:
	void clear();
	void initCL(const ulong_t particleCount, const float stepSize);
	void initGL(const uint_t particleCount, const uint_t width, const uint_t height, const char* name);
	void initParticles(const ulong_t particleCount);
	void periodic();
	void processStep();
	void swap();

	cl::CommandQueue			m_commandQueue;
	cl::Context					m_context;
	cl::Device					m_device;
	GLuint						m_glProgram;
	const ulong_t 				m_particleCount;
	DoubleBuffer<cl::Buffer>	m_particlesProcessingBuffer;
	NBodyKernel					m_particleProcessor;
	DoubleBuffer<GLuint>		m_particlesDrawBuffer;
	std::unique_ptr<Particle[]> m_particlesHostBuffer;
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

