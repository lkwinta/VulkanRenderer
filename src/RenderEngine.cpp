//
/// @author Lukasz
/// @date 23.08.2023

#include "RenderEngine.h"

MyRenderer::RenderEngine::RenderEngine(uint32_t width, uint32_t height, const std::string &title) {
    m_Width = width;
    m_Height = height;
    m_Title = title;

    m_VulkanInstance = new VulkanInstance(ENABLE_VALIDATION_LAYERS);
    m_VulkanDebugMessenger = new VulkanDebugMessenger(ENABLE_VALIDATION_LAYERS);
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
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow((int)m_Width,(int)m_Height, m_Title.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);
}

void MyRenderer::RenderEngine::initVulkan() {
    m_VulkanInstance->SetAppName(m_Title);
    m_VulkanInstance->SetEngineName("RenderEngine");
    m_VulkanInstance->AddExtensions(getRequiredExtensions());
    m_VulkanInstance->AddValidationLayers({"VK_LAYER_KHRONOS_validation"});
    m_VulkanInstance->Create();

    m_VulkanDebugMessenger->Create(m_VulkanInstance->Instance);

    if(createVkSurfaceKHR() != VK_SUCCESS)
        throw std::runtime_error("Failed to create surface!");
    if(createVkPhysicalDevice() != VK_SUCCESS)
        throw std::runtime_error("Failed to create physical device!");
    if(createVkLogicalDevice() != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device!");
    if(createVkSwapChain() != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain!");
    if(createVkSwapChainImageViews() != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain image views!");
    if(createRenderPass() != VK_SUCCESS)
        throw std::runtime_error("Failed to create render pass!");
    if(createVkGraphicsPipeline() != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline!");
    if(createVkFrameBuffers() != VK_SUCCESS)
        throw std::runtime_error("Failed to create framebuffers!");
    if(createVkCommandPool() != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool!");
    if(createVkCommandBuffers() != VK_SUCCESS)
        throw std::runtime_error("Failed to create command buffer!");
    if(createVkSynchronizationObjects() != VK_SUCCESS)
        throw std::runtime_error("Failed to create synchronization objects!");
}

void MyRenderer::RenderEngine::recreateSwapChain() {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);
    while(width == 0 || height == 0){
        glfwGetFramebufferSize(m_Window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_LogicalDevice);

    cleanupSwapChain();

    if(createVkSwapChain() != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain!");
    if(createVkSwapChainImageViews() != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain image views!");
    if(createVkFrameBuffers() != VK_SUCCESS)
        throw std::runtime_error("Failed to create framebuffers!");
}

void MyRenderer::RenderEngine::mainLoop() {
    while(!glfwWindowShouldClose(m_Window)){
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(m_LogicalDevice);
}

void MyRenderer::RenderEngine::cleanup() {
    cleanupSwapChain();

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        vkDestroySemaphore(m_LogicalDevice, m_ImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_LogicalDevice, m_RenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_LogicalDevice, m_InFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, nullptr);
    vkDestroyPipeline(m_LogicalDevice, m_GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
    vkDestroyRenderPass(m_LogicalDevice, m_RenderPass, nullptr);

    vkDestroyDevice(m_LogicalDevice, nullptr);

    if(ENABLE_VALIDATION_LAYERS)
        delete m_VulkanDebugMessenger;

    vkDestroySurfaceKHR(m_VulkanInstance->Instance, m_SurfaceKHR, nullptr);

    delete m_VulkanInstance;

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void MyRenderer::RenderEngine::cleanupSwapChain() {
    for(auto& framebuffer : m_SwapChainFrameBuffers)
        vkDestroyFramebuffer(m_LogicalDevice, framebuffer, nullptr);

    for(auto& imageView : m_SwapChainImageViews)
        vkDestroyImageView(m_LogicalDevice, imageView, nullptr);

    vkDestroySwapchainKHR(m_LogicalDevice, m_SwapChainKHR, nullptr);
}

void MyRenderer::RenderEngine::drawFrame() {
    vkWaitForFences(m_LogicalDevice, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_LogicalDevice, 1, &m_InFlightFences[m_CurrentFrame]);

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(m_LogicalDevice,
                                            m_SwapChainKHR,
                                            UINT64_MAX,
                                            m_ImageAvailableSemaphores[m_CurrentFrame],
                                            VK_NULL_HANDLE, &imageIndex);

    if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to acquire swap chain image!");

    vkResetFences(m_LogicalDevice, 1, &m_InFlightFences[m_CurrentFrame]);

    vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrame], 0);

    recordCommandBuffer(m_CommandBuffers[m_CurrentFrame], imageIndex);

    VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrame]};
    VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrame]};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrame];

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    submitInfo.pWaitDstStageMask = waitStages;

    if(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS){
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChains[] = {m_SwapChainKHR};

    VkPresentInfoKHR presentInfoKhr{};
    presentInfoKhr.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfoKhr.waitSemaphoreCount = 1;
    presentInfoKhr.pWaitSemaphores = signalSemaphores;
    presentInfoKhr.swapchainCount = 1;
    presentInfoKhr.pSwapchains = swapChains;
    presentInfoKhr.pImageIndices = &imageIndex;
    presentInfoKhr.pResults = nullptr;

    result = vkQueuePresentKHR(m_PresentQueue, &presentInfoKhr);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FrameBufferResized){
        m_FrameBufferResized = false;
        recreateSwapChain();
    }
    else if(result != VK_SUCCESS)
        throw std::runtime_error("Failed to present queue!");

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool MyRenderer::RenderEngine::checkDeviceExtensionsSupport(VkPhysicalDevice physicalDevice) {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    std::vector<std::string> availableExtensionNames(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

    for(const auto& extension : availableExtensions){
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

std::vector<const char *> MyRenderer::RenderEngine::getRequiredExtensions() const {
    /*get extensions required by glfw*/
    uint32_t glfwExtensionsCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    /*get extensions required by glfw*/

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions+glfwExtensionsCount);

    if(ENABLE_VALIDATION_LAYERS)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    /*List available extensions*/
    uint32_t availableExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
    std::vector<std::string> availableExtensionNames;

    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

    if(ENABLE_VALIDATION_LAYERS)
        std::cout << "Available Extensions: " << std::endl;

    for(auto extension : availableExtensions){
        if(ENABLE_VALIDATION_LAYERS)
            std::cout << "\t" << extension.extensionName << std::endl;

        availableExtensionNames.emplace_back(extension.extensionName);
    }
    /*List available extensions*/

    /*Check support of required extensions*/
    for(auto extension : extensions)
        if(std::find(availableExtensionNames.begin(), availableExtensionNames.end(), extension) == availableExtensionNames.end())
            throw std::runtime_error(std::string(extension) + " NOT SUPPORTED");

    return extensions;
}

int MyRenderer::RenderEngine::ratePhysicalDevice(const VkPhysicalDevice& physicalDevice) {
    int score = 0;

    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkPhysicalDeviceFeatures physicalDeviceFeatures;

    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

    std::cout << physicalDeviceProperties.deviceName << std::endl;

    if(physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    score += (int)physicalDeviceProperties.limits.maxImageDimension2D;

    if(!physicalDeviceFeatures.geometryShader)
        return 0;

    if(!checkDeviceExtensionsSupport(physicalDevice))
        return 0;

    SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupportDetails(physicalDevice);
    if(swapChainSupportDetails.presentModes.empty() || swapChainSupportDetails.surfaceFormats.empty())
        return 0;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    score += indices.isComplete() * 100;

    return score;
}

MyRenderer::RenderEngine::QueueFamilyIndices MyRenderer::RenderEngine::findQueueFamilies(VkPhysicalDevice physicalDevice) {
    QueueFamilyIndices indices;

    uint32_t queueFamiliesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamiliesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, queueFamiliesProperties.data());

    int i = 0;
    for(const auto& queueFamilyProperties : queueFamiliesProperties){
        if(queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT){
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_SurfaceKHR, &presentSupport);

        if(presentSupport)
            indices.presentFamily = i;

        if(indices.isComplete())
            break; //all queue families found
        i++;
    }

    return indices;
}

MyRenderer::RenderEngine::SwapChainSupportDetails MyRenderer::RenderEngine::querySwapChainSupportDetails(VkPhysicalDevice physicalDevice) {
    SwapChainSupportDetails swapChainDetails{};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_SurfaceKHR, &swapChainDetails.surfaceCapabilities);

    uint32_t formatsCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_SurfaceKHR, &formatsCount, nullptr);
    if(formatsCount != 0){
        swapChainDetails.surfaceFormats.resize(formatsCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_SurfaceKHR, &formatsCount, swapChainDetails.surfaceFormats.data());
    }

    uint32_t presentModesCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_SurfaceKHR, &presentModesCount, nullptr);
    if(presentModesCount != 0){
        swapChainDetails.presentModes.resize(presentModesCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_SurfaceKHR, &presentModesCount, swapChainDetails.presentModes.data());
    }

    return  swapChainDetails;
}

VkSurfaceFormatKHR MyRenderer::RenderEngine::chooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats){
    for(const auto& surfaceFormat : availableSurfaceFormats){
        if(surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return surfaceFormat;
    }

    return availableSurfaceFormats[0];
}

VkPresentModeKHR MyRenderer::RenderEngine::chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes){
    for(const auto& presentMode : availablePresentModes){
        if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return presentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D MyRenderer::RenderEngine::chooseSwapChainExtent2D(const VkSurfaceCapabilitiesKHR& surfaceCapabilities){
    if(surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()){
        return surfaceCapabilities.currentExtent;
    } else {
        int width;
        int height;

        glfwGetFramebufferSize(m_Window, &width, &height);

        VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

        return actualExtent;
    }
}

VkShaderModule MyRenderer::RenderEngine::createShaderModule(const std::vector<char>& shaderCode){
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = static_cast<uint32_t>(shaderCode.size());
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkShaderModule shaderModule;
    if(vkCreateShaderModule(m_LogicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module!");

    return shaderModule;
}

void MyRenderer::RenderEngine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex){
    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin command buffer recording!");

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = m_RenderPass;
    renderPassBeginInfo.framebuffer = m_SwapChainFrameBuffers[imageIndex];

    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = m_SwapChainExtent2D;

    VkClearValue clearValue = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_SwapChainExtent2D.width);
    viewport.height = static_cast<float>(m_SwapChainExtent2D.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissors{};
    scissors.offset = {0, 0};
    scissors.extent = m_SwapChainExtent2D;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissors);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

VkResult MyRenderer::RenderEngine::createVkSurfaceKHR() {
    return glfwCreateWindowSurface(m_VulkanInstance->Instance, m_Window, nullptr, &m_SurfaceKHR);
}

VkResult MyRenderer::RenderEngine::createVkPhysicalDevice() {
    uint32_t availableDevicesCount = 0;
    vkEnumeratePhysicalDevices(m_VulkanInstance->Instance, &availableDevicesCount, nullptr);

    if(availableDevicesCount == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> availableDevices(availableDevicesCount);
    vkEnumeratePhysicalDevices(m_VulkanInstance->Instance, &availableDevicesCount, availableDevices.data());

    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
    int bestScore = 0;
    for(const auto &device : availableDevices){
        int temp = ratePhysicalDevice(device);

        if(temp > bestScore){
            bestScore = temp;
            bestDevice = device;
        }
    }

    m_PhysicalDevice = bestDevice;

    if(m_PhysicalDevice == VK_NULL_HANDLE)
        return VK_ERROR_INITIALIZATION_FAILED;
    else
        return VK_SUCCESS;
}

VkResult MyRenderer::RenderEngine::createVkLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> logicalDeviceQueueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;

    for(auto queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo logicalDeviceQueueCreateInfo{};
        logicalDeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        logicalDeviceQueueCreateInfo.queueFamilyIndex = queueFamily;
        logicalDeviceQueueCreateInfo.queueCount = 1;
        logicalDeviceQueueCreateInfo.pQueuePriorities = &queuePriority;

        logicalDeviceQueueCreateInfos.push_back(logicalDeviceQueueCreateInfo);
    }


    VkPhysicalDeviceFeatures physicalDeviceFeatures{};

    VkDeviceCreateInfo logicalDeviceCreateInfo{};
    logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    logicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(logicalDeviceQueueCreateInfos.size());
    logicalDeviceCreateInfo.pQueueCreateInfos = logicalDeviceQueueCreateInfos.data();

    logicalDeviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

    logicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
    logicalDeviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

    if(ENABLE_VALIDATION_LAYERS){
        logicalDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        logicalDeviceCreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();
    } else {
        logicalDeviceCreateInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(m_PhysicalDevice, &logicalDeviceCreateInfo, nullptr, &m_LogicalDevice);
    if(result == VK_SUCCESS) {
        vkGetDeviceQueue(m_LogicalDevice, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_LogicalDevice, indices.presentFamily.value(), 0, &m_PresentQueue);
    }

    return result;
}

VkResult MyRenderer::RenderEngine::createVkSwapChain(){
    SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupportDetails(m_PhysicalDevice);

    VkSurfaceFormatKHR surfaceFormatKhr = chooseSwapChainSurfaceFormat(swapChainSupportDetails.surfaceFormats);
    VkPresentModeKHR presentModeKhr = chooseSwapChainPresentMode(swapChainSupportDetails.presentModes);
    VkExtent2D extent2D = chooseSwapChainExtent2D(swapChainSupportDetails.surfaceCapabilities);

    uint32_t imageCount = swapChainSupportDetails.surfaceCapabilities.minImageCount + 1;

    if(imageCount > swapChainSupportDetails.surfaceCapabilities.maxImageCount){
        imageCount  = swapChainSupportDetails.surfaceCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = m_SurfaceKHR;

    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageFormat = surfaceFormatKhr.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormatKhr.colorSpace;
    swapChainCreateInfo.presentMode = presentModeKhr;
    swapChainCreateInfo.imageExtent = extent2D;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if(indices.graphicsFamily != indices.presentFamily){
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    swapChainCreateInfo.preTransform = swapChainSupportDetails.surfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.clipped = VK_TRUE;

    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(m_LogicalDevice, &swapChainCreateInfo, nullptr, &m_SwapChainKHR);
    if(result == VK_SUCCESS){
        vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChainKHR, &imageCount, nullptr);
        m_SwapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChainKHR, &imageCount, m_SwapChainImages.data());
        m_SwapChainExtent2D = extent2D;
        m_SwapChainImageFormat = surfaceFormatKhr.format;
    }

    return result;
}

VkResult MyRenderer::RenderEngine::createVkSwapChainImageViews() {
    m_SwapChainImageViews.resize(m_SwapChainImages.size());

    for(size_t i = 0; i < m_SwapChainImageViews.size(); i++){
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = m_SwapChainImages[i];

        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = m_SwapChainImageFormat;

        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        if(vkCreateImageView(m_LogicalDevice, &imageViewCreateInfo, nullptr, &m_SwapChainImageViews[i]) != VK_SUCCESS)
            return VK_ERROR_INITIALIZATION_FAILED;
    }

    return VK_SUCCESS;
}

VkResult MyRenderer::RenderEngine::createRenderPass() {
    VkAttachmentDescription colorAttachmentDescription{};
    colorAttachmentDescription.format = m_SwapChainImageFormat;
    colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;

    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription{};
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;

    VkSubpassDependency subpassDependency{};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;

    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;

    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachmentDescription;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    return vkCreateRenderPass(m_LogicalDevice, &renderPassCreateInfo, nullptr, &m_RenderPass);
}

VkResult MyRenderer::RenderEngine::createVkGraphicsPipeline() {
    auto vertexShaderCode = readFile("shaders/triangle.vert.bin");
    auto fragmentShaderCode = readFile("shaders/triangle.frag.bin");

    VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo{};
    vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

    vertexShaderStageCreateInfo.module = vertexShaderModule;
    vertexShaderStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo{};
    fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    fragmentShaderStageCreateInfo.module = fragmentShaderModule;
    fragmentShaderStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.viewportCount = 1;

    std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCreateInfo.minSampleShading = 1.0f;
    multisampleStateCreateInfo.pSampleMask = nullptr;
    multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable = VK_FALSE;

    /*colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;*/

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

   if(vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
       throw std::runtime_error("Failed to create pipeline layout!");

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = shaderStageCreateInfos;

    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

    graphicsPipelineCreateInfo.layout = m_PipelineLayout;

    graphicsPipelineCreateInfo.renderPass = m_RenderPass;
    graphicsPipelineCreateInfo.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_GraphicsPipeline);

    vkDestroyShaderModule(m_LogicalDevice, vertexShaderModule, nullptr);
    vkDestroyShaderModule(m_LogicalDevice, fragmentShaderModule, nullptr);

    return result;
}

VkResult MyRenderer::RenderEngine::createVkFrameBuffers(){
    m_SwapChainFrameBuffers.resize(m_SwapChainImageViews.size());

    for(size_t i = 0; i < m_SwapChainFrameBuffers.size(); i++){
        VkImageView attachments[] = { m_SwapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = m_RenderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = m_SwapChainExtent2D.width;
        framebufferCreateInfo.height = m_SwapChainExtent2D.height;
        framebufferCreateInfo.layers = 1;

        if(vkCreateFramebuffer(m_LogicalDevice, &framebufferCreateInfo, nullptr, &m_SwapChainFrameBuffers[i]) != VK_SUCCESS)
            return VK_ERROR_INITIALIZATION_FAILED;
    }

    return VK_SUCCESS;
}

VkResult MyRenderer::RenderEngine::createVkCommandPool() {
    QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);

    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();

    return vkCreateCommandPool(m_LogicalDevice, &commandPoolCreateInfo, nullptr, &m_CommandPool);
}

VkResult MyRenderer::RenderEngine::createVkCommandBuffers(){
    m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = m_CommandPool;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    return vkAllocateCommandBuffers(m_LogicalDevice, &commandBufferAllocateInfo, m_CommandBuffers.data());
}

VkResult MyRenderer::RenderEngine::createVkSynchronizationObjects() {
    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        if(vkCreateSemaphore(m_LogicalDevice, &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS)
            return VK_ERROR_INITIALIZATION_FAILED;
        if(vkCreateSemaphore(m_LogicalDevice, &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS)
            return VK_ERROR_INITIALIZATION_FAILED;
        if(vkCreateFence(m_LogicalDevice, &fenceCreateInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
            return VK_ERROR_INITIALIZATION_FAILED;
    }
    return VK_SUCCESS;
}

VkBool32 MyRenderer::RenderEngine::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                 VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                 const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                 void *pUserData) {
    if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void MyRenderer::RenderEngine::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto app = reinterpret_cast<RenderEngine*>(glfwGetWindowUserPointer(window));
    app->m_FrameBufferResized = true;
}


