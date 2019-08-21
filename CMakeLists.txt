# CMakeList.txt : CMake project for NBody, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project(NBody CXX)

IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

find_package(OpenCL REQUIRED)

find_package(OpenGL REQUIRED)
set(OpenGL_INCLUDE_DIRS "${OPENGL_INCLUDE_DIRS}")

add_subdirectory(dependencies/glew EXCLUDE_FROM_ALL)
set(glew_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glew/include")
add_library(glew ALIAS libglew_shared)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
add_subdirectory(dependencies/glfw EXCLUDE_FROM_ALL)
set(glfw_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw/include")
	

# Add source to this project's executable.
add_executable(NBody "NBody.cpp" "NBody.h")
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

# TODO: Add tests and install targets if needed.
