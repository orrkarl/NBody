# CMakeList.txt : CMake project for NBody, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

find_package(OpenCL REQUIRED)
find_package(OpenGL REQUIRED)

# Add source to this project's executable.
add_executable (NBody "NBody.cpp" "NBody.h")

# TODO: Add tests and install targets if needed.
