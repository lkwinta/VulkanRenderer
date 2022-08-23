#include "RenderEngine.h"

int main() {
    auto application = new MyRenderer::RenderEngine(800, 600, "Vulkan Renderer");
    application->Run();
    delete application;
    return 0;
}