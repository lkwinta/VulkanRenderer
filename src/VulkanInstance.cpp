//
// Created by Lukasz on 07.09.2022.
//

#include "VulkanInstance.h"

void VulkanInstance::Create() {
    if(m_ValidationLayersEnabled && !checkValidationLayersSupport())
        throw std::runtime_error("VulkanInstance::VulkanInstance() -> Validation layers requested, but not available!");

    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = m_AppName.c_str();
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = m_EngineName.c_str();
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_0;
    applicationInfo.pNext = nullptr;

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_Extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = m_Extensions.data();

    instanceCreateInfo.flags = 0;

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
    if(m_ValidationLayersEnabled){
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();

        VulkanDebugMessenger::PopulateDebugMessengerCreateInfo(debugMessengerCreateInfo);
        instanceCreateInfo.pNext = &debugMessengerCreateInfo;
    } else {
        instanceCreateInfo.enabledLayerCount = 0;
        instanceCreateInfo.pNext = nullptr;
    }

    if(vkCreateInstance(&instanceCreateInfo, nullptr, &Instance) != VK_SUCCESS)
        throw std::runtime_error("VulkanInstance::VulkanInstance() -> Failed to create instance!");
}

VulkanInstance::~VulkanInstance() {
    vkDestroyInstance(Instance, nullptr);
}

void VulkanInstance::SetAppName(const std::string &name) {
    m_AppName = name;
}

void VulkanInstance::SetEngineName(const std::string &name) {
    m_EngineName = name;
}

bool VulkanInstance::checkValidationLayersSupport() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    std::vector<std::string> availableLayerNames(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    if(m_ValidationLayersEnabled)
        std::cout << "Available Layers: " << std::endl;

    for(auto layer : availableLayers){
        if(m_ValidationLayersEnabled)
            std::cout << "\t" << layer.layerName << std::endl;

        availableLayerNames.emplace_back(layer.layerName);
    }
    for(auto layer : m_ValidationLayers){
        if(std::find(availableLayerNames.begin(), availableLayerNames.end(), layer) == availableLayerNames.end())
            return false;
    }
    return true;
}

void VulkanInstance::AddExtensions(const std::vector<const char*>& extensions) {
    m_Extensions.insert(m_Extensions.end(), extensions.begin(), extensions.end());
}

void VulkanInstance::AddValidationLayers(const std::vector<const char*>& layers) {
    m_ValidationLayers.insert(m_ValidationLayers.end(), layers.begin(), layers.end());
}

