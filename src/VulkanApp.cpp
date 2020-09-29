/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Connor Haskins $
   ======================================================================== */

#include "VulkanApp.h"
#include "Platform.h"
#include "Utils.cpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <stdexcept>
#include <iostream>

struct QueueFamilyIndices {
    uint32 graphics;
    uint32 present;
};

struct SwapChain {
    VkSwapchainKHR handle;
    VkFormat format;
    VkExtent2D extent;
    VkImage* images;
    uint32 imageCount;
    VkImageView* imageViews;
    uint32 framebufferCount;
    VkFramebuffer* framebuffers;
};

struct VulkanContext {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkDevice logicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    SwapChain swapChain;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    uint32 commandBufferCount;
    VkCommandBuffer* commandBuffers;
    VkSemaphore* imageAvailableSemaphores;
    VkSemaphore* renderFinishedSemaphores;
    uint32 currentFrame;
    VkFence* inFlightFences;
    VkFence* imagesInFlight;
};

void initWindow(GLFWwindow** window);
void initVulkan(GLFWwindow* window,VulkanContext* vulkanContext);
bool32 checkValidationLayerSupport();
void initVulkanInstance(VkInstance* vulkanInstance);
void printAvailableExtensions();
void mainLoop(GLFWwindow* window);
void cleanup(GLFWwindow* window, VulkanContext* vulkanContext);
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT createInfo);
bool32 checkValidationLayerSupport();
void initDebugMessenger(VkInstance *instance, VkDebugUtilsMessengerEXT* debugMessenger);
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);
VkResult initDebugUtilsMessengerEXT(VkInstance* instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance* instance,
                                   VkDebugUtilsMessengerEXT* debugMessenger,
                                   const VkAllocationCallbacks* pAllocator);
bool32 findQueueFamilies(VkSurfaceKHR* surface, VkPhysicalDevice* device, QueueFamilyIndices* queueFamilyIndices);
void drawFrame(VulkanContext* vulkanContext);

const uint32 WIDTH = 800;
const uint32 HEIGHT = 600;
const uint32 MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NOT_DEBUG
bool32 enableValidationLayers = false;
#else
bool32 enableValidationLayers = true;
#endif

const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

const char* vertexShaderFileLoc = "vert.spv";
const char* fragmentShaderFileLoc = "frag.spv";
const VkAllocationCallbacks* nullAllocator = nullptr;

void mainLoop(GLFWwindow* window, VulkanContext* vulkanContext) {
    // command buffer recording
    for (uint32 i = 0; i < vulkanContext->commandBufferCount; ++i) {
        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = 0; // Optional
        commandBufferBeginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(vulkanContext->commandBuffers[i], &commandBufferBeginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = vulkanContext->renderPass;
        renderPassBeginInfo.framebuffer = vulkanContext->swapChain.framebuffers[i];
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = vulkanContext->swapChain.extent;

        VkClearValue clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassBeginInfo.clearValueCount = 1; // we can have a clear value for each attachment
        renderPassBeginInfo.pClearValues = &clearValue;

        vkCmdBeginRenderPass(vulkanContext->commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(vulkanContext->commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanContext->graphicsPipeline);

            vkCmdDraw(vulkanContext->commandBuffers[i],
                      3, // vertex count
                      1, // instance count
                      0, // vertex buffer offset
                      0);// instance buffer offset
        }
        vkCmdEndRenderPass(vulkanContext->commandBuffers[i]);

        if (vkEndCommandBuffer(vulkanContext->commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame(vulkanContext);
    }

    vkDeviceWaitIdle(vulkanContext->logicalDevice);
}

void drawFrame(VulkanContext* vulkanContext) {
    vkWaitForFences(vulkanContext->logicalDevice, 1, &vulkanContext->inFlightFences[vulkanContext->currentFrame], VK_TRUE, UINT64_MAX);
    
    uint32 imageIndex; // index inside swapChain.images
    vkAcquireNextImageKHR(vulkanContext->logicalDevice,
                          vulkanContext->swapChain.handle,
                          UINT64_MAX,
                          vulkanContext->imageAvailableSemaphores[vulkanContext->currentFrame],
                          VK_NULL_HANDLE,
                          &imageIndex);

    if(vulkanContext->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(vulkanContext->logicalDevice, 1, &vulkanContext->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    vulkanContext->imagesInFlight[imageIndex] = vulkanContext->inFlightFences[vulkanContext->currentFrame];
    vkResetFences(vulkanContext->logicalDevice, 1, &vulkanContext->inFlightFences[vulkanContext->currentFrame]);
    
    VkSemaphore waitSemaphores[] = {vulkanContext->imageAvailableSemaphores[vulkanContext->currentFrame]}; // which semaphores to wait for
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}; // what stages of the corresponding semaphores to wait for
    VkSemaphore signalSemaphores[] = {vulkanContext->renderFinishedSemaphores[vulkanContext->currentFrame]}; // which semaphores to signal when completed
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkanContext->commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vulkanContext->graphicsQueue, 1, &submitInfo, vulkanContext->inFlightFences[vulkanContext->currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
    
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vulkanContext->swapChain.handle;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // returns a list of results to determine which swap chain may have failed

    vkQueuePresentKHR(vulkanContext->presentQueue, &presentInfo);

    vulkanContext->currentFrame = (++vulkanContext->currentFrame) % MAX_FRAMES_IN_FLIGHT;
}

void initLogicalDevice(VkDevice* device, VkPhysicalDevice* physicalDevice, QueueFamilyIndices* queueFamilyIndices) {

    const real32 queuePriority = 1.0f;

    uint32 uniqueQueueCount = 0;
    VkDeviceQueueCreateInfo queueCIs[2];
    
    VkDeviceQueueCreateInfo graphicsQueueCI{};
    graphicsQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueCI.queueFamilyIndex = queueFamilyIndices->graphics;
    graphicsQueueCI.queueCount = 1;
    graphicsQueueCI.pQueuePriorities = &queuePriority;
    queueCIs[uniqueQueueCount++] = graphicsQueueCI;

    if(queueFamilyIndices->graphics != queueFamilyIndices->present) {
        VkDeviceQueueCreateInfo presentQueueCI = graphicsQueueCI;
        presentQueueCI.queueFamilyIndex = queueFamilyIndices->present;
        queueCIs[uniqueQueueCount++] = presentQueueCI;
    }
    
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceCI{};
    deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCI.pQueueCreateInfos = queueCIs;
    deviceCI.queueCreateInfoCount = uniqueQueueCount;
    deviceCI.pEnabledFeatures = &deviceFeatures;

    deviceCI.enabledExtensionCount = ArrayCount(deviceExtensions);
    deviceCI.ppEnabledExtensionNames = deviceExtensions;
    

    if (enableValidationLayers) {
        deviceCI.enabledLayerCount = ArrayCount(validationLayers);
        deviceCI.ppEnabledLayerNames = validationLayers;
    } else {
        deviceCI.enabledLayerCount = 0;
    }

    if(vkCreateDevice(*physicalDevice, &deviceCI, nullAllocator, device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
}

void initSurface(GLFWwindow* window, VkInstance* instance, VkSurfaceKHR* surface) {
    if (glfwCreateWindowSurface(*instance, window, nullptr, surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void runVulkanApp() {
    GLFWwindow* window;
    VulkanContext vulkanContext = VulkanContext{};
    
    initWindow(&window);
    initVulkan(window, &vulkanContext);
    mainLoop(window, &vulkanContext);
    cleanup(window, &vulkanContext);
}

void initWindow(GLFWwindow** window) {
    glfwInit();
    // Do not create OpenGL context when we ask to create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    *window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr/*which monitor*/, nullptr);
}

bool32 findQueueFamilies(VkSurfaceKHR* surface, VkPhysicalDevice* device, QueueFamilyIndices* queueFamilyIndices) {
    bool32 result = false;
    uint32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*device, &queueFamilyCount, nullptr);

    VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(*device, &queueFamilyCount, queueFamilies);

    VkBool32 present = false;
    bool32 graphics = false;
    for (uint32 i = 0; i < queueFamilyCount; ++i) {
        if(!present) {
            vkGetPhysicalDeviceSurfaceSupportKHR(*device, i, *surface, &present);
            if (present) {
                queueFamilyIndices->present = i;
            }
        }

        if (!graphics && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndices->graphics = i;
            graphics = true;
        }

        if(present && graphics) { break; }
    }

    delete[] queueFamilies;
    return present && graphics;
}

bool32 checkPhysicalDeviceExtensionSupport(VkPhysicalDevice* device){
    uint32 extensionCount;
    vkEnumerateDeviceExtensionProperties(*device, nullptr, &extensionCount, nullptr);
    VkExtensionProperties* availableExtensions = new VkExtensionProperties[extensionCount];
    vkEnumerateDeviceExtensionProperties(*device, nullptr, &extensionCount, availableExtensions);

    uint32 supportedExtensions = 0;
    for(uint32 i = 0; i < ArrayCount(deviceExtensions); ++i) {
        if(supportedExtensions < i) break;
        for(uint32 j = 0; j < extensionCount; ++j) {
            if(strcmp(deviceExtensions[i], availableExtensions[j].extensionName) == 0) {
                ++supportedExtensions;
                break;
            }
        }
    }

    delete[] availableExtensions;
    return supportedExtensions == ArrayCount(deviceExtensions);
}

bool32 checkPhysicalDeviceSwapChainSupport(VkPhysicalDevice* physicalDevice, VkSurfaceKHR* surface) {
    uint32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(*physicalDevice, *surface, &formatCount, nullptr);

    uint32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(*physicalDevice, *surface, &presentModeCount, nullptr);

    // ensure we have any format and some present mode for use
    return formatCount > 0 && presentModeCount > 0;
}

void initSwapChain(VkSurfaceKHR* surface, VkPhysicalDevice* physicalDevice, VkDevice* logicalDevice, QueueFamilyIndices* queueFamilyIndices, SwapChain* swapChain) {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*physicalDevice, *surface, &capabilities);

    uint32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(*physicalDevice, *surface, &formatCount, nullptr);
    VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(*physicalDevice, *surface, &formatCount, surfaceFormats);

    VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];
    for (uint32 i = 0; i < formatCount; ++i) {
        if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = surfaceFormats[i];
            break;
        }
    }
    swapChain->format = surfaceFormat.format;

    uint32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(*physicalDevice, *surface, &presentModeCount, nullptr);
    VkPresentModeKHR* presentModes = new VkPresentModeKHR[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(*physicalDevice, *surface, &presentModeCount, presentModes);

    VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR; // guaranteed to be available
    for(uint32 i = 0; i < presentModeCount; ++i) {
        if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) { // allows triple buffering
            selectedPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    swapChain->extent = capabilities.currentExtent;
    // indication from window manager that we may have different extent dims than the resolution of the window/surface
    if (capabilities.currentExtent.width == UINT32_MAX) {
        swapChain->extent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, WIDTH));
        swapChain->extent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, HEIGHT));
    }

    uint32 imageCount = capabilities.minImageCount + 1;
    if(capabilities.maxImageCount != 0 // there is an image limit
       && capabilities.maxImageCount < imageCount) { // our current count is above that limit
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCI{};
    swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCI.surface = *surface;
    swapChainCI.minImageCount = imageCount;
    swapChainCI.imageFormat = surfaceFormat.format;
    swapChainCI.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCI.imageExtent = swapChain->extent;
    swapChainCI.imageArrayLayers = 1; // 1 unless creating stereoscopic 3D application
    swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32 createFamilyIndices[] = {queueFamilyIndices->graphics, queueFamilyIndices->present};
    if(queueFamilyIndices->graphics != queueFamilyIndices->present) {
        swapChainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCI.queueFamilyIndexCount = 2;
        swapChainCI.pQueueFamilyIndices = createFamilyIndices;
    } else {
        swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCI.queueFamilyIndexCount = 1;
        swapChainCI.pQueueFamilyIndices = createFamilyIndices;
    }

    swapChainCI.preTransform = capabilities.currentTransform;
    swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore blending with other windows on the system
    swapChainCI.presentMode = selectedPresentMode;
    swapChainCI.clipped = VK_TRUE; // ignore pixels obscured by other windows
    swapChainCI.oldSwapchain = VK_NULL_HANDLE; // used in the event that our swap chain needs to be recreated at runtime

    if (vkCreateSwapchainKHR(*logicalDevice, &swapChainCI, nullAllocator, &swapChain->handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    
    vkGetSwapchainImagesKHR(*logicalDevice, swapChain->handle, &swapChain->imageCount, nullptr);
    swapChain->images = new VkImage[swapChain->imageCount];
    vkGetSwapchainImagesKHR(*logicalDevice, swapChain->handle, &swapChain->imageCount, swapChain->images);
    
    delete[] surfaceFormats;
    delete[] presentModes;
}

void pickPhysicalDevice(VkInstance* vulkanInstance, VkSurfaceKHR* surface, VkPhysicalDevice* physicalDevice, QueueFamilyIndices* queueFamilyIndices) {
    *physicalDevice = VK_NULL_HANDLE;
    uint32 deviceCount = 0;
    vkEnumeratePhysicalDevices(*vulkanInstance, &deviceCount, nullptr);

    if(deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[deviceCount];
    vkEnumeratePhysicalDevices(*vulkanInstance, &deviceCount, physicalDevices);

    
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    for(uint32 i = 0; i < deviceCount; ++i) {
        VkPhysicalDevice potentialDevice = physicalDevices[i];
        
        vkGetPhysicalDeviceProperties(potentialDevice, &deviceProperties);
        vkGetPhysicalDeviceFeatures(potentialDevice, &deviceFeatures);

        // TODO: More complex device selection if needed
        bool32 isDeviceSuitable = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
            && findQueueFamilies(surface, &physicalDevices[i], queueFamilyIndices)
            && checkPhysicalDeviceExtensionSupport(&potentialDevice)
            && checkPhysicalDeviceSwapChainSupport(&potentialDevice, surface);

        if(isDeviceSuitable){
            *physicalDevice = physicalDevices[i];
            break;
        }
    }

    if(physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    delete[] physicalDevices;
}

void initImageViews(VkDevice* logicalDevice, SwapChain* swapChain) {
    swapChain->imageViews = new VkImageView[swapChain->imageCount];
    for(uint32 i = 0; i < swapChain->imageCount; ++i) {
        VkImageViewCreateInfo imageViewCI{};
        imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCI.image = swapChain->images[i];
        imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D; // how to treat images (opts: 1D, 2D, 3D, cube maps)
        imageViewCI.format = swapChain->format;
        imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCI.subresourceRange.baseMipLevel = 0;
        imageViewCI.subresourceRange.levelCount = 1;
        imageViewCI.subresourceRange.baseArrayLayer = 0;
        imageViewCI.subresourceRange.layerCount = 1;
        if (vkCreateImageView(*logicalDevice, &imageViewCI, nullAllocator, &swapChain->imageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void initGraphicsPipeline(VkDevice* logicalDevice, VkExtent2D* extent, VkPipelineLayout* pipelineLayout, VkRenderPass* renderPass, VkPipeline* graphicsPipeline) {
    uint32 vertShaderSize;
    char* vertexShaderFile;
    VkShaderModule vertexShaderModule;

    readFile(vertexShaderFileLoc, &vertShaderSize, nullptr);
    vertexShaderFile = new char[vertShaderSize];
    readFile(vertexShaderFileLoc, &vertShaderSize, vertexShaderFile);

    VkShaderModuleCreateInfo vertShaderModuleCI{};
    vertShaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertShaderModuleCI.codeSize = vertShaderSize;
    vertShaderModuleCI.pCode = (const uint32*) vertexShaderFile; // Note: there may be concerns with data alignment
    if(vkCreateShaderModule(*logicalDevice, &vertShaderModuleCI, nullAllocator, &vertexShaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    VkPipelineShaderStageCreateInfo vertShaderStageCI{};
    vertShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageCI.module = vertexShaderModule;
    vertShaderStageCI.pName = "main";
    
    char* fragmentShaderFile;
    uint32 fragShaderSize;
    VkShaderModule fragmentShaderModule;
    
    readFile(fragmentShaderFileLoc, &fragShaderSize, nullptr);
    fragmentShaderFile = new char[fragShaderSize];
    readFile(fragmentShaderFileLoc, &fragShaderSize, fragmentShaderFile);

    VkShaderModuleCreateInfo fragShaderModuleCI{};
    fragShaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragShaderModuleCI.codeSize = fragShaderSize;
    fragShaderModuleCI.pCode = (const uint32*) fragmentShaderFile;
    if(vkCreateShaderModule(*logicalDevice, &fragShaderModuleCI, nullAllocator, &fragmentShaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    
    VkPipelineShaderStageCreateInfo fragShaderStageCI{};
    fragShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageCI.module = fragmentShaderModule;
    fragShaderStageCI.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageCI, fragShaderStageCI};

    VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
    vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCI.vertexBindingDescriptionCount = 0;
    vertexInputStateCI.pVertexBindingDescriptions = nullptr;
    vertexInputStateCI.vertexAttributeDescriptionCount = 0;
    vertexInputStateCI.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
    inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCI.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (real32) extent->width;
    viewport.height = (real32) extent->height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = *extent;

    VkPipelineViewportStateCreateInfo viewportStateCI{};
    viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCI.viewportCount = 1;
    viewportStateCI.pViewports = &viewport;
    viewportStateCI.scissorCount = 1;
    viewportStateCI.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
    rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCI.depthClampEnable = VK_FALSE;
    rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCI.lineWidth = 1.0f;
    rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationStateCI.depthBiasEnable = VK_FALSE;
    rasterizationStateCI.depthBiasConstantFactor = 0.0f;
    rasterizationStateCI.depthBiasClamp = 0.0f;
    rasterizationStateCI.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
    multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCI.sampleShadingEnable = VK_FALSE;
    multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCI.minSampleShading = 1.0f; 
    multisampleStateCI.pSampleMask = nullptr; 
    multisampleStateCI.alphaToCoverageEnable = VK_FALSE; 
    multisampleStateCI.alphaToOneEnable = VK_FALSE; 

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; 
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; 
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD; 
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; 
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; 
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD; 

    VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
    colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCI.logicOpEnable = VK_FALSE;
    colorBlendStateCI.logicOp = VK_LOGIC_OP_COPY; 
    colorBlendStateCI.attachmentCount = 1;
    colorBlendStateCI.pAttachments = &colorBlendAttachmentState;
    colorBlendStateCI.blendConstants[0] = 0.0f;
    colorBlendStateCI.blendConstants[1] = 0.0f;
    colorBlendStateCI.blendConstants[2] = 0.0f;
    colorBlendStateCI.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = 0; 
    pipelineLayoutCI.pSetLayouts = nullptr; 
    pipelineLayoutCI.pushConstantRangeCount = 0; 
    pipelineLayoutCI.pPushConstantRanges = nullptr; 

    if (vkCreatePipelineLayout(*logicalDevice, &pipelineLayoutCI, nullAllocator, pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.stageCount = 2;
    pipelineCI.pStages = shaderStages;
    pipelineCI.pVertexInputState = &vertexInputStateCI;
    pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
    pipelineCI.pViewportState = &viewportStateCI;
    pipelineCI.pRasterizationState = &rasterizationStateCI;
    pipelineCI.pMultisampleState = &multisampleStateCI;
    pipelineCI.pDepthStencilState = nullptr; 
    pipelineCI.pColorBlendState = &colorBlendStateCI;
    pipelineCI.pDynamicState = nullptr;
    pipelineCI.layout = *pipelineLayout;
    pipelineCI.renderPass = *renderPass;
    pipelineCI.subpass = 0;
    pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCI.basePipelineIndex = -1;
    
    if (vkCreateGraphicsPipelines(*logicalDevice, VK_NULL_HANDLE, 1, &pipelineCI, nullAllocator, graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    
    vkDestroyShaderModule(*logicalDevice, vertexShaderModule, nullAllocator);
    vkDestroyShaderModule(*logicalDevice, fragmentShaderModule, nullAllocator);
    
    delete[] vertexShaderFile;
    delete[] fragmentShaderFile;
}

void initRenderPass(VkDevice* logicalDevice, SwapChain* swapChain, VkRenderPass* renderPass) {
    VkAttachmentDescription colorAttachmentDesc{};
    colorAttachmentDesc.format = swapChain->format;
    colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription supbassDesc{};
    supbassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    supbassDesc.colorAttachmentCount = 1;
    // NOTE: The index of the attachments in this array directly referenced in the fragment shader
    // ex: "layout(location = 0) out vec4 outColor"
    supbassDesc.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency subpassDependency{};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo renderPassCI{};
    renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCI.attachmentCount = 1;
    renderPassCI.pAttachments = &colorAttachmentDesc;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &supbassDesc;
    renderPassCI.dependencyCount = 1;
    renderPassCI.pDependencies = &subpassDependency;

    if (vkCreateRenderPass(*logicalDevice, &renderPassCI, nullAllocator, renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void initFramebuffers(VkDevice* logicalDevice, SwapChain* swapChain, VkRenderPass* renderPass) {
    swapChain->framebufferCount = swapChain->imageCount;
    swapChain->framebuffers = new VkFramebuffer[swapChain->framebufferCount];

    for (uint32 i = 0; i < swapChain->framebufferCount; i++) {
        VkImageView attachments[] = {swapChain->imageViews[i]};

        VkFramebufferCreateInfo framebufferCI{};
        framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCI.renderPass = *renderPass;
        framebufferCI.attachmentCount = 1;
        framebufferCI.pAttachments = attachments;
        framebufferCI.width = swapChain->extent.width;
        framebufferCI.height = swapChain->extent.height;
        framebufferCI.layers = 1;

        if (vkCreateFramebuffer(*logicalDevice, &framebufferCI, nullAllocator, &swapChain->framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void initCommandPool(VkDevice* logicalDevice, VkCommandPool* commandPool, QueueFamilyIndices* queueFamilyIndices) {
    VkCommandPoolCreateInfo commandPoolCI{};
    commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCI.queueFamilyIndex = queueFamilyIndices->graphics;
    commandPoolCI.flags = 0;

    if (vkCreateCommandPool(*logicalDevice, &commandPoolCI, nullAllocator, commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void initCommandBuffers(VkDevice* logicalDevice, VkCommandPool* commandPool, VkCommandBuffer** commandBuffers, uint32* commandBufferCount, SwapChain* swapChain) {
    *commandBufferCount = swapChain->framebufferCount;
    *commandBuffers = new VkCommandBuffer[*commandBufferCount];

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = *commandPool;
    // Primary level allows us to submit directly to a queue for execution, secondary level allows us to reference from another command buffer
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = *commandBufferCount;

    if (vkAllocateCommandBuffers(*logicalDevice, &commandBufferAllocateInfo, *commandBuffers) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void initSyncObjects(VkDevice* logicalDevice, VkSemaphore** imageAvailableSemaphores, VkSemaphore** renderFinishedSemaphores, VkFence** inFlightFences, VkFence** imagesInFlight, SwapChain* swapChain) {
    *imageAvailableSemaphores = new VkSemaphore[MAX_FRAMES_IN_FLIGHT];
    *renderFinishedSemaphores = new VkSemaphore[MAX_FRAMES_IN_FLIGHT];
    *inFlightFences = new VkFence[MAX_FRAMES_IN_FLIGHT];
    *imagesInFlight = new VkFence[swapChain->imageCount];

    VkSemaphoreCreateInfo semaphoreCI{};
    semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCI{};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(*logicalDevice, &semaphoreCI, nullAllocator, &(*imageAvailableSemaphores)[i]) != VK_SUCCESS ||
            vkCreateSemaphore(*logicalDevice, &semaphoreCI, nullAllocator, &(*renderFinishedSemaphores)[i]) != VK_SUCCESS ||
            vkCreateFence(*logicalDevice, &fenceCI, nullAllocator, &(*inFlightFences)[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects!");
        }
    }

    for(uint32 i = 0; i < swapChain->imageCount; ++i) {
        (*imagesInFlight)[i] = VK_NULL_HANDLE;
    }
}

void initVulkan(GLFWwindow* window, VulkanContext* vulkanContext) {
    vulkanContext->currentFrame = 0;
    
    initVulkanInstance(&vulkanContext->instance);
    initDebugMessenger(&vulkanContext->instance, &vulkanContext->debugMessenger);
    initSurface(window, &vulkanContext->instance, &vulkanContext->surface);
    
    VkPhysicalDevice physicalDevice;
    QueueFamilyIndices queueFamilyIndices{};

    pickPhysicalDevice(&vulkanContext->instance, &vulkanContext->surface, &physicalDevice, &queueFamilyIndices);
    initLogicalDevice(&vulkanContext->logicalDevice, &physicalDevice, &queueFamilyIndices);
    vkGetDeviceQueue(vulkanContext->logicalDevice, queueFamilyIndices.graphics, 0, &vulkanContext->graphicsQueue);
    vkGetDeviceQueue(vulkanContext->logicalDevice, queueFamilyIndices.present, 0, &vulkanContext->presentQueue);
    initSwapChain(&vulkanContext->surface, &physicalDevice, &vulkanContext->logicalDevice, &queueFamilyIndices, &vulkanContext->swapChain);
    initImageViews(&vulkanContext->logicalDevice, &vulkanContext->swapChain);
    initRenderPass(&vulkanContext->logicalDevice, &vulkanContext->swapChain, &vulkanContext->renderPass);
    initGraphicsPipeline(&vulkanContext->logicalDevice, &vulkanContext->swapChain.extent, &vulkanContext->pipelineLayout, &vulkanContext->renderPass, &vulkanContext->graphicsPipeline);
    initFramebuffers(&vulkanContext->logicalDevice, &vulkanContext->swapChain, &vulkanContext->renderPass);
    initCommandPool(&vulkanContext->logicalDevice, &vulkanContext->commandPool, &queueFamilyIndices);
    initCommandBuffers(&vulkanContext->logicalDevice, &vulkanContext->commandPool, &vulkanContext->commandBuffers, &vulkanContext->commandBufferCount, &vulkanContext->swapChain);
    initSyncObjects(&vulkanContext->logicalDevice, &vulkanContext->imageAvailableSemaphores, &vulkanContext->renderFinishedSemaphores, &vulkanContext->inFlightFences, &vulkanContext->imagesInFlight, &vulkanContext->swapChain);
}

bool32 checkValidationLayerSupport() {
    uint32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        
    VkLayerProperties* availableLayers = new VkLayerProperties[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    bool32 layersFound = false;
    for (uint32 i = 0; i < ArrayCount(validationLayers); i++) {
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

VkResult initDebugUtilsMessengerEXT(VkInstance* instance,
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

void initDebugMessenger(VkInstance *instance, VkDebugUtilsMessengerEXT* debugMessenger) {
    if(!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI;
    populateDebugMessengerCreateInfo(&debugUtilsMessengerCI);

    VkResult result = initDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCI, nullAllocator, debugMessenger);
    if(result != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void initVulkanInstance(VkInstance* vulkanInstance) {
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

    VkInstanceCreateInfo instanceCI{};
    instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCI.pApplicationInfo = &appInfo;

    uint32 extensionsCount;
    getRequiredExtensions(nullptr, &extensionsCount);
    const char** extensions = new const char*[extensionsCount];
    getRequiredExtensions(extensions, &extensionsCount);
    instanceCI.enabledExtensionCount = extensionsCount;
    instanceCI.ppEnabledExtensionNames = extensions;
    instanceCI.enabledLayerCount = 0;

    VkDebugUtilsMessengerCreateInfoEXT debugUtilMessengerCI;
    if(enableValidationLayers) {
        instanceCI.enabledLayerCount = ArrayCount(validationLayers);
        instanceCI.ppEnabledLayerNames = validationLayers;

        populateDebugMessengerCreateInfo(&debugUtilMessengerCI);
        instanceCI.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugUtilMessengerCI;
    } else {
        instanceCI.enabledLayerCount = 0;
        instanceCI.pNext = nullptr;
    }
    
    VkResult result = vkCreateInstance(
        &instanceCI, // poiter to struct with creation info
        nullAllocator, // pointer to custom allocator callacks
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

    delete[] extensions;
}

void cleanup(GLFWwindow* window, VulkanContext* vulkanContext) {
    if(enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(&vulkanContext->instance, &vulkanContext->debugMessenger, nullAllocator);
    }

    vkDestroyCommandPool(vulkanContext->logicalDevice, vulkanContext->commandPool, nullAllocator);
    
    for(uint32 i = 0; i < vulkanContext->swapChain.framebufferCount; ++i) {
        vkDestroyFramebuffer(vulkanContext->logicalDevice, vulkanContext->swapChain.framebuffers[i], nullAllocator);
    }
    
    for(uint32 i = 0; i < vulkanContext->swapChain.imageCount; ++i) {
        vkDestroyImageView(vulkanContext->logicalDevice, vulkanContext->swapChain.imageViews[i], nullAllocator);
    }

    for(uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(vulkanContext->logicalDevice, vulkanContext->renderFinishedSemaphores[i], nullAllocator);
        vkDestroySemaphore(vulkanContext->logicalDevice, vulkanContext->imageAvailableSemaphores[i], nullAllocator);
        vkDestroyFence(vulkanContext->logicalDevice, vulkanContext->inFlightFences[i], nullAllocator);
    }
    
    vkDestroyRenderPass(vulkanContext->logicalDevice, vulkanContext->renderPass, nullAllocator);
    vkDestroyPipeline(vulkanContext->logicalDevice, vulkanContext->graphicsPipeline, nullAllocator);
    vkDestroyPipelineLayout(vulkanContext->logicalDevice, vulkanContext->pipelineLayout, nullAllocator);
    vkDestroySwapchainKHR(vulkanContext->logicalDevice, vulkanContext->swapChain.handle, nullAllocator);
    vkDestroySurfaceKHR(vulkanContext->instance, vulkanContext->surface, nullAllocator);
    vkDestroyDevice(vulkanContext->logicalDevice, nullAllocator);
    vkDestroyInstance(vulkanContext->instance, nullAllocator);
    
    delete[] vulkanContext->swapChain.images;
    delete[] vulkanContext->swapChain.framebuffers;
    delete[] vulkanContext->commandBuffers;
    delete[] vulkanContext->renderFinishedSemaphores;
    delete[] vulkanContext->imageAvailableSemaphores;
    
    glfwDestroyWindow(window);
    glfwTerminate();
}
