//
// Created by Lukasz on 23.08.2022.
//
#include "RenderEngine.h"

MyRenderer::RenderEngine::RenderEngine(uint32_t width, uint32_t height, const std::string &title) {
    m_Width = width;
    m_Height = height;
    m_Title = title;
}


void MyRenderer::RenderEngine::Run() {
    initWindow(); //initiate GLFW Window
    initVulkan();
    mainLoop();
    cleanup();
}

void MyRenderer::RenderEngine::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow((int)m_Width,(int)m_Height, m_Title.c_str(), nullptr, nullptr);
}

void MyRenderer::RenderEngine::initVulkan() {

}
void MyRenderer::RenderEngine::mainLoop() {
    while(!glfwWindowShouldClose(m_Window)){
        glfwPollEvents();
    }
}

void MyRenderer::RenderEngine::cleanup() {
    glfwDestroyWindow(m_Window);



    glfwTerminate();
}

