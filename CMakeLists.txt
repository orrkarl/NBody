# CMakeList.txt : CMake project for NBody, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project(NBody CXX)

set(CMAKE_CXX_STANDARD 17)

IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

find_package(OpenCL REQUIRED)

find_package(OpenGL REQUIRED)
set(OpenGL_INCLUDE_DIRS "${OPENGL_INCLUDE_DIRS}")

add_subdirectory(dependencies/glew EXCLUDE_FROM_ALL)
set(glew_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glew/include/GL")
add_library(glew ALIAS libglew_shared)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
add_subdirectory(dependencies/glfw EXCLUDE_FROM_ALL)
set(glfw_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw/include")
	

# Add source to this project's executable.
add_executable(
	NBody 
	"src/cl.cpp" "src/cl.h" 
	"src/common.cpp" "src/common.h" 
	"src/DoubleBuffer.h" 
	"src/main.cpp" 
	"src/NBody.cpp" "src/NBody.h" 
	"src/NBodyKernel.h" 
	"src/NBodyProgram.cpp" "src/NBodyProgram.h"
	"src/Particle.h"
)

target_link_libraries(NBody 
	PUBLIC ${OpenCL_LIBRARIES}
	PUBLIC ${OpenGL_LIBRARIES}
	PUBLIC glfw
	PUBLIC glew)
target_include_directories(NBody
	PUBLIC ${OpenCL_INCLUDE_DIRS}
	PUBLIC ${OpenGL_INCLUDE_DIRS}
	PUBLIC ${glew_INCLUDE_DIRS}
	PUBLIC ${glfw_INCLUDE_DIRS})


if(WIN32)
	if(CMAKE_BUILD_TYPE STREQUAL Debug)
			add_custom_command(
				TARGET NBody
				PRE_LINK
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"${PROJECT_BINARY_DIR}/bin/glewd.dll"         
				${CMAKE_CURRENT_BINARY_DIR}
			)	
		else()
			add_custom_command(
				TARGET NBody
				PRE_LINK
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"${PROJECT_BINARY_DIR}/bin/glew.dll"         
				${CMAKE_CURRENT_BINARY_DIR}
			)
	endif()
endif()


# TODO: Add tests and install targets if needed.
