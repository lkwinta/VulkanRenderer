//
// Created by Lukasz on 08.09.2022.
//

#include "VulkanDebugMessenger.h"

VulkanDebugMessenger::~VulkanDebugMessenger() {
    if(m_ValidationLayersEnabled)
        vkDestroyDebugUtilsMessengerEXT(m_VkInstance, DebugMessengerEXT, nullptr);
}

void VulkanDebugMessenger::Create(const VkInstance& instance) {
    if(!m_ValidationLayersEnabled) return;

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
    VulkanDebugMessenger::PopulateDebugMessengerCreateInfo(debugMessengerCreateInfo);

    m_VkInstance = instance;

    if(vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerCreateInfo, nullptr, &DebugMessengerEXT) != VK_SUCCESS)
        throw std::runtime_error("VulkanDebugMessenger::Create() -> Failed to create debug messenger!");
}

void VulkanDebugMessenger::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &debugUtilsMessengerCreateInfoExt) {
    debugUtilsMessengerCreateInfoExt.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugUtilsMessengerCreateInfoExt.messageSeverity =  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugUtilsMessengerCreateInfoExt.messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugUtilsMessengerCreateInfoExt.pfnUserCallback = debugCallback;
    debugUtilsMessengerCreateInfoExt.flags = 0;

    debugUtilsMessengerCreateInfoExt.pUserData = nullptr;
    debugUtilsMessengerCreateInfoExt.pNext = nullptr;
}

VkResult VulkanDebugMessenger::vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                                  const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                                  const VkAllocationCallbacks *pAllocator,
                                                                  VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if(func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanDebugMessenger::vkDestroyDebugUtilsMessengerEXT( VkInstance instance,
                                                                VkDebugUtilsMessengerEXT debugMessenger,
                                                                const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if(func != nullptr)
        func(instance, debugMessenger, pAllocator);
}

VkBool32 VulkanDebugMessenger::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                       VkDebugUtilsMessageTypeFlagsEXT messageType,
                                       const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                       void *pUserData) {
    if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}
