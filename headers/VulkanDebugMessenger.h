//
// Created by Lukasz on 08.09.2022.
//

#ifndef VULKANRENDERER_VULKANDEBUGMESSENGER_H
#define VULKANRENDERER_VULKANDEBUGMESSENGER_H

#include <vulkan/vulkan.h>
#include <VulkanInstance.h>

#include <cstdint>

class VulkanDebugMessenger {
public:
    VkDebugUtilsMessengerEXT DebugMessengerEXT = VK_NULL_HANDLE;

    VulkanDebugMessenger(bool validationEnabled) : m_ValidationLayersEnabled(validationEnabled) {};
    ~VulkanDebugMessenger();

    void Create(const VkInstance& instance);
    static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCreateInfoExt);

private:
    [[ nodiscard ]] static VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                                   const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                                   const VkAllocationCallbacks* pAllocator,
                                                                   VkDebugUtilsMessengerEXT* pDebugMessenger);

    static void vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                                VkDebugUtilsMessengerEXT debugMessenger,
                                                const VkAllocationCallbacks* pAllocator);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData
    );

    const bool m_ValidationLayersEnabled;
    VkInstance m_VkInstance;
};


#endif //VULKANRENDERER_VULKANDEBUGMESSENGER_H
