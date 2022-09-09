//
// Created by Lukasz on 30.08.2022.
//


#ifndef VULKANRENDERER_VULKANINSTANCE_H
#define VULKANRENDERER_VULKANINSTANCE_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#include <VulkanDebugMessenger.h>

class VulkanInstance {
public:
    explicit VulkanInstance(bool validationEnabled) : m_ValidationLayersEnabled(validationEnabled){};
    ~VulkanInstance();

    void Create();

    void SetAppName(const std::string &name);
    void SetEngineName(const std::string &name);

    void AddExtensions(const std::vector<const char*>& extensions);
    void AddValidationLayers(const std::vector<const char*>& layers);

    VkInstance Instance = nullptr;
private:
    bool checkValidationLayersSupport();

    std::string m_EngineName = "Default";
    std::string m_AppName = "Default";

    std::vector<const char*> m_Extensions;
    std::vector<const char*> m_ValidationLayers;

    const bool m_ValidationLayersEnabled;
};

#endif //VULKANRENDERER_VULKANINSTANCE_H
