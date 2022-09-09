//
/// @author Lukasz
//

#ifndef VULKANRENDERER_RENDERENGINE_H
#define VULKANRENDERER_RENDERENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <optional>
#include <set>
#include <limits>
#include <fstream>

#include "VulkanInstance.h"
#include "VulkanDebugMessenger.h"

static std::vector<char> readFile(const std::string& filename){
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if(!file.is_open())
        throw std::runtime_error("Failed to open a file: " + filename + " !");

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), (long long)fileSize);

    file.close();

    return buffer;
}

namespace MyRenderer{
    class RenderEngine{
    public:
        RenderEngine(uint32_t width, uint32_t height, const std::string& title);
        ~RenderEngine() = default;

        void Run();

    private:
        const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
        uint32_t m_CurrentFrame = 0;
        bool m_FrameBufferResized = false;
        // ********** STRUCTS *********** //

        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;

            ///@brief checks if all queue families has value
            [[nodiscard]] bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
        };

        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR surfaceCapabilities;
            std::vector<VkSurfaceFormatKHR> surfaceFormats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        // **********MAIN CORE*********** //

        ///@brief Initializes GLFW Window
        void initWindow();

        ///@brief calls all functions which creates Vulkan objects
        void initVulkan();

        void recreateSwapChain();

        ///@brief main loop - glfwPollEvents
        void mainLoop();

        ///@todo move to destructor?
        ///@brief basically destructor
        void cleanup();

        void cleanupSwapChain();

        void drawFrame();

        // **********MAIN CORE*********** //

        // ********HELPER METHODS******** //
        bool checkDeviceExtensionsSupport(VkPhysicalDevice physicalDevice);

        [[nodiscard]] std::vector<const char*> getRequiredExtensions() const;
        int ratePhysicalDevice(const VkPhysicalDevice &physicalDevice);

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);
        SwapChainSupportDetails querySwapChainSupportDetails(VkPhysicalDevice physicalDevice);

        static VkSurfaceFormatKHR chooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats);
        static VkPresentModeKHR chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapChainExtent2D(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

        VkShaderModule createShaderModule(const std::vector<char>& shaderCode);

        void recordCommandBuffer(VkCommandBuffer, uint32_t imageIndex);
        // ********HELPER METHODS******** //

        ////////////EXTENSION FUNCTIONS////////////

        ////////////EXTENSION FUNCTIONS////////////

        [[ nodiscard ]] VkResult createVkSurfaceKHR();
        [[ nodiscard ]] VkResult createVkPhysicalDevice();
        [[ nodiscard ]] VkResult createVkLogicalDevice();
        [[ nodiscard ]] VkResult createVkSwapChain();
        [[ nodiscard ]] VkResult createVkSwapChainImageViews();
        [[ nodiscard ]] VkResult createRenderPass();
        [[ nodiscard ]] VkResult createVkGraphicsPipeline();
        [[ nodiscard ]] VkResult createVkFrameBuffers();
        [[ nodiscard ]] VkResult createVkCommandPool();
        [[ nodiscard ]] VkResult createVkCommandBuffers();
        [[ nodiscard ]] VkResult createVkSynchronizationObjects();

        ///////////////CALLBACKS///////////////////
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                    void* pUserData
                );
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
        ///////////////CALLBACKS///////////////////
    private:
        uint32_t m_Width;
        uint32_t m_Height;

        std::string m_Title;

        GLFWwindow* m_Window;

        std::vector<const char*> m_ValidationLayers = {
                "VK_LAYER_KHRONOS_validation"
        };

        std::vector<const char*> m_DeviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        std::vector<VkImage> m_SwapChainImages = {};
        std::vector<VkImageView> m_SwapChainImageViews = {};
        std::vector<VkFramebuffer> m_SwapChainFrameBuffers = {};

#ifdef NDEBUG
        const bool ENABLE_VALIDATION_LAYERS = false;
#else
        const bool ENABLE_VALIDATION_LAYERS = true;
#endif
        VulkanInstance* m_VulkanInstance;
        VulkanDebugMessenger* m_VulkanDebugMessenger;

        /*Vulkan objects*/
        VkSurfaceKHR m_SurfaceKHR = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_LogicalDevice = VK_NULL_HANDLE;
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue  = VK_NULL_HANDLE;
        VkSwapchainKHR m_SwapChainKHR = VK_NULL_HANDLE;
        VkFormat m_SwapChainImageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D m_SwapChainExtent2D{};
        VkRenderPass m_RenderPass{};
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_CommandBuffers = {};

        std::vector<VkSemaphore> m_ImageAvailableSemaphores = {};
        std::vector<VkSemaphore> m_RenderFinishedSemaphores = {};
        std::vector<VkFence> m_InFlightFences = {};
    };
}


#endif //VULKANRENDERER_RENDERENGINE_H
