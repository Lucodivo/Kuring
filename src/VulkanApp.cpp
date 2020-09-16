/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Connor Haskins $
   ======================================================================== */

#include "VulkanApp.h"

#include <cstdlib>
#include <stdexcept>
#include <iostream>

void runVulkanApp() {
    GLFWwindow* window;
    VkInstance vulkanInstance;
    VkDebugUtilsMessengerEXT debugMessenger;
    
    initWindow(&window);
    initVulkan(&vulkanInstance, &debugMessenger);
    mainLoop(window);
    cleanup(window, &vulkanInstance, &debugMessenger);
}

void initWindow(GLFWwindow** window) {
    glfwInit();
    // Do not create OpenGL context when we ask to create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    *window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr/*which monitor*/, nullptr);
}

void initVulkan(VkInstance* vulkanInstance, VkDebugUtilsMessengerEXT* debugMessenger) {
    createVulkanInstance(vulkanInstance);
    setupDebugMessenger(vulkanInstance, debugMessenger);
}

bool checkValidationLayerSupport() {
    uint32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        
    VkLayerProperties* availableLayers = new VkLayerProperties[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    bool layersFound;
    for (uint32 i = 0; i < validationLayerCount; i++) {
        layersFound = false;

        const char* validationLayer = validationLayers[i];
        for (uint32 j = 0; j < layerCount; j++) {
            char* availableLayer = availableLayers[j].layerName;
            if (strcmp(validationLayer, availableLayer) == 0) {
                layersFound = true;
                break;
            }
        }

        if (!layersFound) {
            break;
        }
    }

    delete[] availableLayers;
    return layersFound;
}

void getRequiredExtensions(const char ** extensions, uint32 *extensionCount) {
    uint32 glfwExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    *extensionCount = enableValidationLayers ? glfwExtensionCount + 1 : glfwExtensionCount;
    
    if(extensions == nullptr) return;
    
    // copy GLFW required extensions
    for(uint32 i = 0; i < glfwExtensionCount; ++i) {
        extensions[i] = glfwExtensions[i];
    }

    // append debug utils extension
    if(enableValidationLayers) {
        extensions[*extensionCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance* instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(*instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance* instance,
                                   VkDebugUtilsMessengerEXT* debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
       func(*instance, *debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo) {
    *createInfo = {};
    createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo->pfnUserCallback = debugCallback;
}

void setupDebugMessenger(VkInstance *instance, VkDebugUtilsMessengerEXT* debugMessenger) {
    if(!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(&createInfo);

    VkResult result = CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, debugMessenger);
    if(result != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void createVulkanInstance(VkInstance* vulkanInstance) {
    if(enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested but not found");
    }
    
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32 extensionsCount;
    getRequiredExtensions(nullptr, &extensionsCount);
    const char** extensions = new const char*[extensionsCount];
    getRequiredExtensions(extensions, &extensionsCount);
    createInfo.enabledExtensionCount = extensionsCount;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = 0;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if(enableValidationLayers) {
        createInfo.enabledLayerCount = validationLayerCount;
        createInfo.ppEnabledLayerNames = validationLayers;

        populateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }
    
    VkResult result = vkCreateInstance(
        &createInfo, // poiter to struct with creation info
        nullptr, // pointer to custom allocator callacks
        vulkanInstance); // pointer to variable that stores the handle to new object

    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    printAvailableExtensions();

    delete[] extensions;
}
    
void printAvailableExtensions() {
    uint32 extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    VkExtensionProperties* extensions = new VkExtensionProperties[extensionCount];
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);

    std::cout << "available extensions:\n";
    for(uint32 i = 0; i < extensionCount; i++) {
        std::cout << '\t' << extensions[i].extensionName << '\n';
    }
}

void mainLoop(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void cleanup(GLFWwindow* window, VkInstance* vulkanInstance, VkDebugUtilsMessengerEXT* debugMessenger) {
    if(enableValidationLayers) {
        //DestroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, nullptr);
    }

    vkDestroyInstance(*vulkanInstance, nullptr);
        
    glfwDestroyWindow(window);
    glfwTerminate();
}
