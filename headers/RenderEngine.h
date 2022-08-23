//
// Created by Lukasz on 23.08.2022.
//

#ifndef VULKANRENDERER_RENDERENGINE_H
#define VULKANRENDERER_RENDERENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

namespace MyRenderer{
    class RenderEngine{
    public:
        RenderEngine(uint32_t width, uint32_t height, const std::string& title);
        ~RenderEngine() = default;

        void Run();

    private:
        /*Inits GLFW Window*/
        void initWindow();
        void initVulkan();
        void mainLoop();
        void cleanup();

    private:
        uint32_t m_Width;
        uint32_t m_Height;

        std::string m_Title;

        GLFWwindow* m_Window;
    };
}


#endif //VULKANRENDERER_RENDERENGINE_H
