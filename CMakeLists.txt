cmake_minimum_required(VERSION 3.22)
project(VulkanRenderer)

set(CMAKE_CXX_STANDARD 14)

find_package(Vulkan REQUIRED)

add_executable(VulkanRenderer src/main.cpp)
target_link_libraries(VulkanRenderer Vulkan::Vulkan)