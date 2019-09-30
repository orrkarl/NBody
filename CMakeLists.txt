cmake_minimum_required(VERSION 3.8)

project(NBody VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Debug")

set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
add_subdirectory(dependencies/glfw EXCLUDE_FROM_ALL)

find_package(Vulkan REQUIRED)

include(cmake/glslc.cmake)

add_executable(triangle src/main.cpp src/util/debug.cpp)
target_link_libraries(triangle glfw Vulkan::Vulkan)
target_include_directories(triangle PRIVATE ${GLFW_INCLUDE_DIRS} PRIVATE Vulkan::Vulkan)

add_shader(triangle src/simple.frag frag.spv)
add_shader(triangle src/simple.vert vert.spv)
