#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

#include "DoubleBuffer.h"
#include "NBodyKernel.h"

class NBody
{
public:
	NBody(const long particaleCount, const float stepSize);

	void run();

private:
	void init();

	void preiodic();

	void destroy();

	cl::CommandQueue			m_commandQueue;
	cl::Context					m_context;
	cl::Device					m_device;
	const long					m_particaleCount;
	DoubleBuffer<cl::BufferGL>	m_particlesProcessingBuffer;
	DoubleBuffer<GLuint>		m_particlesDrawBuffer;
	const float					m_stepSize;
	GLuint						m_vao;
	GLFWwindow*					m_window;
};

