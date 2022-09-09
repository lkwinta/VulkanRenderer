#include "RenderEngine.h"
#include <stdexcept>

int main() {
    auto application = MyRenderer::RenderEngine(800, 600, "Vulkan Renderer");

    //heap allocation throws SIGSEV????
    try{
        application.Run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}