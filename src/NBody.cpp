// NBody.cpp : Defines the entry point for the application.
//

#include "NBody.h"

#include "NBodyProgram.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

const char* VERTEX_SHADER = "";
const int VERTEX_SHADER_LENGTH = strlen(VERTEX_SHADER);

const char* FRAGMENT_SHADER = "";
const int FRAGMENT_SHADER_LENGTH = strlen(FRAGMENT_SHADER);

struct GLShaderRAII
{
public:
	GLShaderRAII(const GLenum shaderType)
		: shader(glCreateShader(shaderType))
	{
	}

	~GLShaderRAII()
	{
		glDeleteShader(shader);
	}

	operator GLuint() { return shader; }


	GLuint shader;
};

void errorCallback(int error, const char* description)
{
	std::cerr << "GLFW Error: " << description << '\n';
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void NBody::initialize()
{
	if (!glfwInit())
	{
		const char* err;
		glfwGetError(&err);
		throw std::runtime_error(err);
	}
	glfwSetErrorCallback(errorCallback);
}

void NBody::destroy() noexcept
{
	glfwTerminate();
}

NBody::NBody(const ulong_t particleCount, const float stepSize, const uint_t width, const uint_t height, const char* name)
	: m_vao(0), m_window(nullptr)
{
	initCL(particleCount, stepSize);
	initGL(particleCount, width, height, name);
}

void NBody::initCL(const ulong_t particleCount, const float stepSize)
{
	auto platform = cl::Platform::get();

	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

	if (!devices.size())
	{
		platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
	}

	if (!devices.size())
	{
		throw std::runtime_error("No OpenCL devices found!");
	}

	m_device = devices[0];

	cl_context_properties props[3]
	{
		CL_CONTEXT_PLATFORM,
		reinterpret_cast<cl_context_properties>(platform()),
		0
	};

	m_context = cl::Context(m_device, props);

	m_commandQueue = cl::CommandQueue(m_context, m_device, (cl_command_queue_properties)CL_QUEUE_PROFILING_ENABLE);

	m_particlesProcessingBuffer.back()  = cl::Buffer(m_context, CL_MEM_READ_WRITE, particleCount * sizeof(Particle));
	m_particlesProcessingBuffer.front() = cl::Buffer(m_context, CL_MEM_READ_WRITE, particleCount * sizeof(Particle));

	auto prog = NBodyProgram(m_context);
	m_particleProcessor = NBodyKernel(prog);
	m_particleProcessor.setParticleCount(particleCount);
	m_particleProcessor.setStepSize(stepSize);
}

void NBody::initGL(const uint_t particleCount, const uint_t width, const uint_t height, const char* name)
{
	const char* glfwErr;
	GLenum glewErr;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

	m_window = glfwCreateWindow(width, height, name, nullptr, nullptr);
	if (!m_window)
	{
		glfwGetError(&glfwErr);
		throw std::runtime_error(std::string("could not create window: ") + glfwErr);
	}

	glfwMakeContextCurrent(m_window);

	glewExperimental = true;
	glewErr = glewInit();
	if (glewErr != GLEW_OK)
	{
		glfwDestroyWindow(m_window);
		throw std::runtime_error(std::to_string(glewErr));
	}

	glfwSetKeyCallback(m_window, keyCallback);
	glfwSwapInterval(1);

	glCreateVertexArrays(1, &m_vao);
	glCreateBuffers(2, m_particlesDrawBuffer.base());
	validateGL();

	glNamedBufferData(m_particlesDrawBuffer.back(), particleCount * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW);
	glNamedBufferData(m_particlesDrawBuffer.front(), particleCount * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW);
	validateGL();

	m_glProgram = glCreateProgram();
	
	auto vShader = GLShaderRAII(GL_VERTEX_SHADER);
	glShaderSource(vShader, 1, &VERTEX_SHADER, &VERTEX_SHADER_LENGTH);
	glCompileShader(vShader);
	validateGL();

	auto fShader = GLShaderRAII(GL_FRAGMENT_SHADER);
	glShaderSource(fShader, 1, &FRAGMENT_SHADER, &FRAGMENT_SHADER_LENGTH);
	glCompileShader(fShader);
	validateGL();

	glAttachShader(m_glProgram, vShader);
	glAttachShader(m_glProgram, fShader);
	glLinkProgram(m_glProgram);
	validateGL();

	glUseProgram(m_glProgram);
}

NBody::~NBody() noexcept
{
	glfwDestroyWindow(m_window);	

	glDeleteBuffers(2, m_particlesDrawBuffer.base());
	glDeleteVertexArrays(1, &m_vao);

	glDeleteProgram(m_glProgram);
}

void NBody::run()
{
	while (!glfwWindowShouldClose(m_window))
	{
		periodic();

		glfwSwapBuffers(m_window);
		glfwPollEvents();
	}
}

void NBody::periodic()
{
	std::cout << "Hello " << std::endl;
	throw std::runtime_error("World!");
}

