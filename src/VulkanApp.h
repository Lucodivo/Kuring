/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Connor Haskins $
   ======================================================================== */

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Platform.h"

void runVulkanApp();
void initWindow(GLFWwindow** window);
void initVulkan(VkInstance* vulkanInstance, VkDebugUtilsMessengerEXT* debugMessenger);
bool checkValidationLayerSupport();
void createVulkanInstance(VkInstance* vulkanInstance);
void printAvailableExtensions();
void mainLoop(GLFWwindow* window);
void cleanup(GLFWwindow* window, VkInstance* vulkanInstance, VkDebugUtilsMessengerEXT* debugMessenger);
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT createInfo);
bool checkValidationLayerSupport();
void setupDebugMessenger(VkInstance *instance, VkDebugUtilsMessengerEXT* debugMessenger);
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);
VkResult CreateDebugUtilsMessengerEXT(VkInstance* instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance* instance,
                                   VkDebugUtilsMessengerEXT* debugMessenger,
                                   const VkAllocationCallbacks* pAllocator);


const uint32 WIDTH = 800;
const uint32 HEIGHT = 600;

#ifdef NOT_DEBUG
bool32 enableValidationLayers = false;
#else
bool32 enableValidationLayers = true;
#endif

const uint32 validationLayerCount = 1;
const char* validationLayers[validationLayerCount] = {"VK_LAYER_KHRONOS_validation"};
