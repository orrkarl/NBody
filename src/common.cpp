
#include "common.h"

#include <stdexcept>
#include <string>

void validateGL()
{
	auto err = glGetError();
	if (err != GL_NO_ERROR)
	{
		throw std::runtime_error(std::to_string(err));
	}
}