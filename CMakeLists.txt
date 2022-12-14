cmake_minimum_required(VERSION 3.22)
project(VulkanRenderer)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

# GLFW
set(GLFW_DIR dependencies/glfw) # Set this to point to an up-to-date GLFW repo
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
add_subdirectory(${GLFW_DIR} binary_dir EXCLUDE_FROM_ALL)
include_directories(${GLFW_DIR}/include)

# Dear ImGui
set(IMGUI_DIR dependencies/imgui)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ..)

#Vulkan
find_package(Vulkan REQUIRED COMPONENTS glslc)

#glm
add_subdirectory(dependencies/glm)

set(LIBRARIES glfw Vulkan::Vulkan glm::glm)

set(IMGUI_SRC
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp)

file(GLOB SOURCE_FILES src/main.cpp
        src/RenderEngine.cpp
        src/VulkanInstance.cpp
        headers/RenderEngine.h
        headers/VulkanInstance.h
        headers/VulkanDebugMessenger.h
        src/VulkanDebugMessenger.cpp)

include_directories(headers)

find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

function(compile_shader target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "ENV;FORMAT" "SOURCES")
    foreach(source ${arg_SOURCES})
        add_custom_command(
                OUTPUT ${source}.${arg_FORMAT}
                DEPENDS ${source}
                DEPFILE ${source}.d
                COMMAND
                ${glslc_executable}
                $<$<BOOL:${arg_ENV}>:--target-env=${arg_ENV}>
                $<$<BOOL:${arg_FORMAT}>:-mfmt=${arg_FORMAT}>
                #-MD -MF ${source}.d
                -o ${source}.${arg_FORMAT}
                ${CMAKE_CURRENT_SOURCE_DIR}/${source}
        )
        target_sources(${target} PRIVATE ${source}.${arg_FORMAT})
    endforeach()
endfunction()

add_executable(VulkanRenderer ${SOURCE_FILES} ${HEADER_FILES} ${IMGUI_SRC})
target_link_libraries(VulkanRenderer ${LIBRARIES})
target_compile_definitions(VulkanRenderer PUBLIC -DImTextureID=ImU64)

compile_shader(VulkanRenderer ENV vulkan1.1 FORMAT bin SOURCES shaders/triangle.vert shaders/triangle.frag)