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
    uint32 transfer;
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
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;
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

    // Vertex buffer and attributes
    struct {
        VkDeviceMemory memory; // Handle to the device memory for this buffer
        VkBuffer buffer;       // Handle to the Vulkan buffer object that the memory is bound to
    } vertices;

    // Index buffer
    struct {
        VkDeviceMemory memory;
        VkBuffer buffer;
        uint32 count;
    } indices;

};

// Vertex layout used in this example
struct Vertex {
    real32 position[3];
    real32 color[3];
};

void initWindow(GLFWwindow** window);
void initVulkan(GLFWwindow* window,VulkanContext* vulkanContext);
bool32 checkValidationLayerSupport();
void initVulkanInstance(VkInstance* instance, VkDebugUtilsMessengerEXT* debugMessenger);
void printAvailableExtensions();
void mainLoop(GLFWwindow* window);
void cleanup(GLFWwindow* window, VulkanContext* vulkanContext);
bool32 checkValidationLayerSupport();
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);
VkResult initDebugUtilsMessengerEXT(
    VkInstance* instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance* instance, VkDebugUtilsMessengerEXT* debugMessenger, const VkAllocationCallbacks* pAllocator);
bool32 findQueueFamilies(VkSurfaceKHR* surface, VkPhysicalDevice* device, QueueFamilyIndices* queueFamilyIndices);
void drawFrame(VulkanContext* vulkanContext);
void getRequiredExtensions(const char ** extensions, uint32 *extensionCount);

const uint32 WIDTH = 800;
const uint32 HEIGHT = 600;
const uint32 MAX_FRAMES_IN_FLIGHT = 2;
const uint32 VERTEX_INPUT_BINDING_INDEX = 0;
const uint64 DEFAULT_FENCE_TIMEOUT = 100000000000;

#ifdef NOT_DEBUG
bool32 enableValidationLayers = false;
#else
bool32 enableValidationLayers = true;
#endif

const char* VALIDATION_LAYERS[] = {"VK_LAYER_KHRONOS_validation"};
const char* DEVICE_EXTENSIONS[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

const char* VERT_SHADER_FILE_LOC = "vert.spv";
const char* FRAG_SHADER_FILE_LOC = "frag.spv";
const char* APP_NAME = "Hello Vulkan";
const char* ENGINE_NAME = "N/A";
const VkAllocationCallbacks* nullAllocator = nullptr;

void mainLoop(GLFWwindow* window, VulkanContext* vulkanContext) {
    // command buffer recording
    for (uint32 i = 0; i < vulkanContext->commandBufferCount; ++i) {
        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = 0;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;

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

            // Bind triangle vertex buffer (contains position and colors)
            VkDeviceSize bufferOffsets[1] = { 0 };
            vkCmdBindVertexBuffers(vulkanContext->commandBuffers[i],
                                   0 /*First binding index as described by VkVertexInputBindingDescription.binding*/,
                                   1 /*Vertex buffer count*/,
                                   &vulkanContext->vertices.buffer /*Vertex buffer array*/,
                                   bufferOffsets /*offset of vertex attributes in the array of buffers*/);

            // Bind triangle index buffer
            vkCmdBindIndexBuffer(vulkanContext->commandBuffers[i],
                                 vulkanContext->indices.buffer,
                                 0/*offset in buffer*/,
                                 VK_INDEX_TYPE_UINT32);

            // Draw indexed triangle
            vkCmdDrawIndexed(vulkanContext->commandBuffers[i], vulkanContext->indices.count, 1, 0, 0, 1);
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

    if (vkQueueSubmit(vulkanContext->graphicsQueue, 1, &submitInfo,
                      vulkanContext->inFlightFences[vulkanContext->currentFrame] /* signaled on completion of all submitted command bufers */
                      ) != VK_SUCCESS) {
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

void initLogicalDeviceAndQueues(VkDevice* device, VkPhysicalDevice* physicalDevice, QueueFamilyIndices* queueFamilyIndices) {
    const real32 queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCIs[2];

    uint32 uniqueQueuesCount = 1;
    VkDeviceQueueCreateInfo graphicsQueueCI{};
    graphicsQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueCI.queueFamilyIndex = queueFamilyIndices->graphics;
    graphicsQueueCI.queueCount = 1;
    graphicsQueueCI.pQueuePriorities = &queuePriority;
    queueCIs[0] = graphicsQueueCI;

    VkDeviceQueueCreateInfo presentQueueCI;
    if(queueFamilyIndices->present != queueFamilyIndices->graphics) {
        presentQueueCI = graphicsQueueCI;
        presentQueueCI.queueFamilyIndex = queueFamilyIndices->present;
        queueCIs[uniqueQueuesCount++] = presentQueueCI;
    }

    VkDeviceQueueCreateInfo transferQueueCI;
    if(queueFamilyIndices->transfer != queueFamilyIndices->graphics &&
       queueFamilyIndices->transfer != queueFamilyIndices->present) {
        transferQueueCI = graphicsQueueCI;
        transferQueueCI.queueFamilyIndex = queueFamilyIndices->transfer;
        queueCIs[uniqueQueuesCount++] = transferQueueCI;
    }

    VkDeviceCreateInfo deviceCI{};
    deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCI.pQueueCreateInfos = queueCIs;
    deviceCI.queueCreateInfoCount = uniqueQueuesCount;
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceCI.pEnabledFeatures = &deviceFeatures;
    deviceCI.enabledExtensionCount = ArrayCount(DEVICE_EXTENSIONS);
    deviceCI.ppEnabledExtensionNames = DEVICE_EXTENSIONS;
    deviceCI.enabledLayerCount = enableValidationLayers ? ArrayCount(VALIDATION_LAYERS) : 0;
    deviceCI.ppEnabledLayerNames = enableValidationLayers ? VALIDATION_LAYERS : nullptr;

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

    bool32 present = false;
    bool32 graphics = false;
    bool32 transfer = false;
    for (uint32 i = 0; i < queueFamilyCount; ++i) {
        if(!present) {
            VkBool32 supportsPresentation = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(*device, i, *surface, &supportsPresentation);
            if (supportsPresentation) {
                present = true;
                queueFamilyIndices->present = i;
            }
        }

        if (!graphics && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndices->graphics = i;
            graphics = true;
        }

        if (!transfer && queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            queueFamilyIndices->transfer = i;
            transfer = true;
        }

        if(present && graphics && transfer) { break; }
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
    for(uint32 i = 0; i < ArrayCount(DEVICE_EXTENSIONS); ++i) {
        if(supportedExtensions < i) break;
        for(uint32 j = 0; j < extensionCount; ++j) {
            if(strcmp(DEVICE_EXTENSIONS[i], availableExtensions[j].extensionName) == 0) {
                ++supportedExtensions;
                break;
            }
        }
    }

    delete[] availableExtensions;
    return supportedExtensions == ArrayCount(DEVICE_EXTENSIONS);
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
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*physicalDevice, *surface, &surfaceCapabilities);

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
            break;
        }
    }

    swapChain->extent = surfaceCapabilities.currentExtent;
    // indication from window manager that we may have different extent dims than the resolution of the window/surface
    if (surfaceCapabilities.currentExtent.width == UINT32_MAX) {
        VkExtent2D minExtent = surfaceCapabilities.minImageExtent;
        VkExtent2D maxExtent = surfaceCapabilities.maxImageExtent;
        swapChain->extent.width = clamp(minExtent.width, maxExtent.width, WIDTH);
        swapChain->extent.height = clamp(minExtent.height, maxExtent.height, HEIGHT);
    }

    uint32 imageCount = surfaceCapabilities.minImageCount + 1;
    if(surfaceCapabilities.maxImageCount != 0 // there is an image limit
       && surfaceCapabilities.maxImageCount < imageCount) { // our current count is above that limit
        imageCount = surfaceCapabilities.maxImageCount;
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

    swapChainCI.preTransform = surfaceCapabilities.currentTransform;
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

void pickPhysicalDevice(VulkanContext* vulkanContext, VkPhysicalDevice* physicalDevice, QueueFamilyIndices* queueFamilyIndices) {
    VkInstance vulkanInstance = vulkanContext->instance;
    VkSurfaceKHR surface = vulkanContext->surface;

    *physicalDevice = VK_NULL_HANDLE;
    uint32 deviceCount = 0;
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);

    if(deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[deviceCount];
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, physicalDevices);
    
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    for(uint32 i = 0; i < deviceCount; ++i) {
        VkPhysicalDevice potentialDevice = physicalDevices[i];
        
        vkGetPhysicalDeviceProperties(potentialDevice, &deviceProperties);
        vkGetPhysicalDeviceFeatures(potentialDevice, &deviceFeatures);

        // TODO: More complex device selection if needed
        bool32 isDeviceSuitable = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
            && findQueueFamilies(&surface, &physicalDevices[i], queueFamilyIndices)
            && checkPhysicalDeviceExtensionSupport(&potentialDevice)
            && checkPhysicalDeviceSwapChainSupport(&potentialDevice, &surface);

        if(isDeviceSuitable){
            *physicalDevice = physicalDevices[i];
            break;
        }
    }

    if(physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    vkGetPhysicalDeviceMemoryProperties(*physicalDevice, &vulkanContext->deviceMemoryProperties);

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
        imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // which aspects of an image are included in a view, in this case color
        imageViewCI.subresourceRange.baseMipLevel = 0;
        imageViewCI.subresourceRange.levelCount = 1;
        imageViewCI.subresourceRange.baseArrayLayer = 0;
        imageViewCI.subresourceRange.layerCount = 1;
        if (vkCreateImageView(*logicalDevice, &imageViewCI, nullAllocator, &swapChain->imageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

// This function is used to request a device memory type that supports all the property flags we request (e.g. device local, host visible)
// Upon success it will return the index of the memory type that fits our requested memory properties
// This is necessary as implementations can offer an arbitrary number of memory types with different
// memory properties.
// You can check http://vulkan.gpuinfo.org/ for details on different memory configurations
uint32 getMemoryTypeIndex(VkPhysicalDeviceMemoryProperties* deviceMemoryProperties, uint32 memoryTypeBits, VkMemoryPropertyFlags properties)
{
    // Iterate over all memory types available for the device used in this example
    for (uint32 i = 0; i < deviceMemoryProperties->memoryTypeCount; i++)
    {
        // memoryTypeBits is a bitmask and contains one bit set for every supported memory type for the resource.
        // Bit i is set if and only if the memory type i in the VkPhysicalDeviceMemoryProperties structure for
        // the physical device is supported for the resource.
        if ((memoryTypeBits & (1 << i)) != 0)
        {
            if ((deviceMemoryProperties->memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
    }

    throw "Could not find a suitable memory type!";
}

// Prepare vertex and index buffers for an indexed triangle
// Also uploads them to device local memory using staging and initializes vertex input and attribute binding to match the vertex shader
void initVertexAttributes(VulkanContext* vulkanContext)
{
    VkDevice device = vulkanContext->logicalDevice;
    VkDeviceMemory* vertexMemory = &vulkanContext->vertices.memory;
    VkBuffer* vertexBuffer = &vulkanContext->vertices.buffer;
    VkDeviceMemory* indexMemory = &vulkanContext->indices.memory;
    VkBuffer* indexBuffer = &vulkanContext->indices.buffer;
    uint32* indexCount = &vulkanContext->indices.count;
    VkPhysicalDeviceMemoryProperties* deviceMemoryProperties = &vulkanContext->deviceMemoryProperties;
    
    // A note on memory management in Vulkan in general:
    //  This is a very complex topic and while it's fine for an example application to small individual memory allocations that is not
    //  what should be done a real-world application, where you should allocate large chunks of memory at once instead.

    // Setup vertices
    Vertex vertexData[] =
        {
            { // VERTEX
                {  0.0f, -0.5f,  0.0f }, // POSITION
                {  1.0f,  0.0f,  0.0f }  // COLOR
            },
            {
                {  0.5f,  0.5f,  0.0f },
                {  0.0f,  1.0f,  0.0f }
            },
            {
                { -0.5f,  0.5f,  0.0f },
                {  0.0f,  0.0f,  1.0f }
            }
        };
    uint32 vertexBufferSize = ArrayCount(vertexData) * sizeof(Vertex);

    // Setup indices
    uint32 indexData[] = { 0, 1, 2 };
    *indexCount = ArrayCount(indexData);
    uint32 indexBufferSize = (*indexCount) * sizeof(uint32);
    
    // Static data like vertex and index buffer should be stored on the device memory
    // for optimal (and fastest) access by the GPU
    //
    // To achieve this we use "staging buffers" :
    // - Create a buffer that's visible to the host (and can be mapped)
    // - Copy the data to this buffer
    // - Create another buffer that's local on the device (VRAM) with the same size
    // - Copy the data from the host to the device using a command buffer
    // - Delete the host visible (staging) buffer
    // - Use the device local buffers for rendering

    // Create a host-visible buffer to copy the vertex data to (staging buffer)
    VkBufferCreateInfo hostVisibleVertexBufferInfo = {};
    hostVisibleVertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    hostVisibleVertexBufferInfo.size = vertexBufferSize;
    hostVisibleVertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // Buffer is used as the copy source

    VkBuffer stagingVertexBuffer;
    vkCreateBuffer(device, &hostVisibleVertexBufferInfo, nullAllocator, &stagingVertexBuffer);
    
    VkMemoryRequirements vertexBufferMemReqs;
    vkGetBufferMemoryRequirements(device, stagingVertexBuffer, &vertexBufferMemReqs);
    
    VkMemoryAllocateInfo vertexBufferMemAllocInfo = {};
    vertexBufferMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vertexBufferMemAllocInfo.allocationSize = vertexBufferMemReqs.size;
    vertexBufferMemAllocInfo.memoryTypeIndex = getMemoryTypeIndex(deviceMemoryProperties, vertexBufferMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDeviceMemory stagingVertexMemory;
    vkAllocateMemory(device, &vertexBufferMemAllocInfo, nullAllocator, &stagingVertexMemory);

    // Map staging memory and copy to staging buffer
    void *stagingVertexMemoryCopyPtr;
    vkMapMemory(device, stagingVertexMemory, 0/*offset*/, vertexBufferMemAllocInfo.allocationSize, 0/*flags*/, &stagingVertexMemoryCopyPtr);
    memcpy(stagingVertexMemoryCopyPtr, vertexData, vertexBufferSize);
    stagingVertexMemoryCopyPtr = nullptr;
    vkUnmapMemory(device, stagingVertexMemory);
    vkBindBufferMemory(device, stagingVertexBuffer, stagingVertexMemory, 0);

    // Create destination buffer with device only visibility
    VkBufferCreateInfo deviceLocalVertexBufferInfo = {};
    deviceLocalVertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    deviceLocalVertexBufferInfo.size = vertexBufferSize;
    deviceLocalVertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; // device local buffer to  be used for rendering
    
    vkCreateBuffer(device, &deviceLocalVertexBufferInfo, nullAllocator, vertexBuffer);

    vkGetBufferMemoryRequirements(device, *vertexBuffer, &vertexBufferMemReqs);
    vertexBufferMemAllocInfo.allocationSize = vertexBufferMemReqs.size; 
    vertexBufferMemAllocInfo.memoryTypeIndex = getMemoryTypeIndex(deviceMemoryProperties, vertexBufferMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(device, &vertexBufferMemAllocInfo, nullAllocator, vertexMemory);
    vkBindBufferMemory(device, *vertexBuffer, *vertexMemory, 0);

    // repeat above for index data
    VkBufferCreateInfo hostVisibleIndexBufferInfo = {};
    hostVisibleIndexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    hostVisibleIndexBufferInfo.size = indexBufferSize;
    hostVisibleIndexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    
    VkBuffer stagingIndexBuffer;
    vkCreateBuffer(device, &hostVisibleIndexBufferInfo, nullAllocator, &stagingIndexBuffer);

    VkMemoryRequirements indexBufferMemReqs;
    vkGetBufferMemoryRequirements(device, stagingIndexBuffer, &indexBufferMemReqs);
    
    VkMemoryAllocateInfo indexBufferMemAllocInfo = {};
    indexBufferMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    indexBufferMemAllocInfo.allocationSize = indexBufferMemReqs.size;
    indexBufferMemAllocInfo.memoryTypeIndex = getMemoryTypeIndex(deviceMemoryProperties, indexBufferMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDeviceMemory stagingIndexMemory;
    vkAllocateMemory(device, &indexBufferMemAllocInfo, nullAllocator, &stagingIndexMemory);

    vkBindBufferMemory(device, stagingIndexBuffer, stagingIndexMemory, 0);
    
    void *stagingIndexMemoryCopyPtr;
    vkMapMemory(device, stagingIndexMemory, 0, indexBufferMemAllocInfo.allocationSize, 0, &stagingIndexMemoryCopyPtr);
    memcpy(stagingIndexMemoryCopyPtr, indexData, indexBufferSize);
    stagingIndexMemoryCopyPtr = nullptr;
    vkUnmapMemory(device, stagingIndexMemory);

    VkBufferCreateInfo deviceLocalIndexBufferInfo = {};
    deviceLocalIndexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    deviceLocalIndexBufferInfo.size = indexBufferSize;
    deviceLocalIndexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    vkCreateBuffer(device, &deviceLocalIndexBufferInfo, nullAllocator, indexBuffer);

    vkGetBufferMemoryRequirements(device, *indexBuffer, &indexBufferMemReqs);
    indexBufferMemAllocInfo.allocationSize = indexBufferMemReqs.size;
    indexBufferMemAllocInfo.memoryTypeIndex = getMemoryTypeIndex(deviceMemoryProperties, indexBufferMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(device, &indexBufferMemAllocInfo, nullAllocator, indexMemory);
    
    vkBindBufferMemory(device, *indexBuffer, *indexMemory, 0);

    // Buffer copies have to be submitted to a queue, so we need a command buffer
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = vulkanContext->commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer copyCommandBuffer;
    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &copyCommandBuffer);

    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = 0;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(copyCommandBuffer, &commandBufferBeginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer for staging vertex/index buffers !");
    }

    // TODO: Use the same buffer and memory chunk for both vertex & index data
    // Put buffer region copies into command buffer
    VkBufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    // Vertex buffer
    copyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(copyCommandBuffer, stagingVertexBuffer, *vertexBuffer, 1/* region count */, &copyRegion);
    // Index buffer
    copyRegion.size = indexBufferSize;
    vkCmdCopyBuffer(copyCommandBuffer, stagingIndexBuffer, *indexBuffer, 1, &copyRegion);

    vkEndCommandBuffer(copyCommandBuffer);

    VkSubmitInfo copyCommandSubmitInfo = {};
    copyCommandSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    copyCommandSubmitInfo.commandBufferCount = 1;
    copyCommandSubmitInfo.pCommandBuffers = &copyCommandBuffer;

    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo copyFenceCreateInfo = {};
    copyFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    copyFenceCreateInfo.flags = 0;
    
    VkFence copyFence;
    vkCreateFence(device, &copyFenceCreateInfo, nullAllocator, &copyFence);

    // Submit to the queue
    vkQueueSubmit(vulkanContext->transferQueue, 1, &copyCommandSubmitInfo, copyFence);
    // Wait for the fence to signal that command buffer has finished executing
    vkWaitForFences(device, 1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);

    vkDestroyFence(device, copyFence, nullAllocator);
    vkFreeCommandBuffers(device, vulkanContext->commandPool, 1, &copyCommandBuffer);

    // Destroy staging buffers
    // Note: Staging buffer must not be deleted before the copies have been submitted and executed
    vkDestroyBuffer(device, stagingVertexBuffer, nullAllocator);
    vkFreeMemory(device, stagingVertexMemory, nullAllocator);
    vkDestroyBuffer(device, stagingIndexBuffer, nullAllocator);
    vkFreeMemory(device, stagingIndexMemory, nullAllocator);
}

void initGraphicsPipeline(VkDevice* logicalDevice, VkExtent2D* extent, VkPipelineLayout* pipelineLayout, VkRenderPass* renderPass, VkPipeline* graphicsPipeline) {

    // Shader initialization
    uint32 vertShaderSize;
    char* vertexShaderFile;
    VkShaderModule vertexShaderModule;

    readFile(VERT_SHADER_FILE_LOC, &vertShaderSize, nullptr);
    vertexShaderFile = new char[vertShaderSize];
    readFile(VERT_SHADER_FILE_LOC, &vertShaderSize, vertexShaderFile);

    VkShaderModuleCreateInfo vertShaderModuleCI{};
    vertShaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertShaderModuleCI.codeSize = vertShaderSize;
    vertShaderModuleCI.pCode = (const uint32*) vertexShaderFile; // Note: there may be concerns with data alignment
    if(vkCreateShaderModule(*logicalDevice, &vertShaderModuleCI, nullAllocator, &vertexShaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    
    char* fragmentShaderFile;
    uint32 fragShaderSize;
    VkShaderModule fragmentShaderModule;
    
    readFile(FRAG_SHADER_FILE_LOC, &fragShaderSize, nullptr);
    fragmentShaderFile = new char[fragShaderSize];
    readFile(FRAG_SHADER_FILE_LOC, &fragShaderSize, fragmentShaderFile);

    VkShaderModuleCreateInfo fragShaderModuleCI{};
    fragShaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragShaderModuleCI.codeSize = fragShaderSize;
    fragShaderModuleCI.pCode = (const uint32*) fragmentShaderFile;
    if(vkCreateShaderModule(*logicalDevice, &fragShaderModuleCI, nullAllocator, &fragmentShaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    VkPipelineShaderStageCreateInfo vertShaderStageCI{};
    vertShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageCI.module = vertexShaderModule;
    vertShaderStageCI.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageCI{};
    fragShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageCI.module = fragmentShaderModule;
    fragShaderStageCI.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageCI, fragShaderStageCI};

    // vertex attributes
    VkVertexInputBindingDescription vertexInputBindingDesc = {};
    // the binding point index is used when describing the attributes and when binding vertex buffers with draw commands
    vertexInputBindingDesc.binding = VERTEX_INPUT_BINDING_INDEX;
    vertexInputBindingDesc.stride = sizeof(Vertex);
    vertexInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Input attribute bindings describe shader attribute locations and memory layouts
    VkVertexInputAttributeDescription vertexInputAttributes[2] = {{0}};
        
    // Attribute location 0: Position
    // GLSL: layout (location = 0) in vec3 inPos;
    vertexInputAttributes[0].binding = VERTEX_INPUT_BINDING_INDEX;
    vertexInputAttributes[0].location = 0;
    // Position attr is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
    vertexInputAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributes[0].offset = offsetof(Vertex, position);
        
    // Attribute location 1: Color
    // GLSL: layout (location = 1) in vec3 inColor;
    vertexInputAttributes[1].binding = VERTEX_INPUT_BINDING_INDEX;
    vertexInputAttributes[1].location = 1;
    // Color attr is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
    vertexInputAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributes[1].offset = offsetof(Vertex, color);

    // Vertex input state used for pipeline creation
    VkPipelineVertexInputStateCreateInfo vertexInputStateCI = {};
    vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCI.vertexBindingDescriptionCount = 1;
    vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBindingDesc;
    vertexInputStateCI.vertexAttributeDescriptionCount = 2;
    vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes;
    
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
    rasterizationStateCI.cullMode = VK_CULL_MODE_NONE; //TODO: VK_CULL_MODE_BACK_BIT;
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
    pipelineLayoutCI.setLayoutCount = 0; // descriptor set layouts
    pipelineLayoutCI.pSetLayouts = nullptr; // num descriptor set layouts
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
    const uint32 colorAttachmentIndex = 0;
    VkAttachmentDescription attachmentDescs[1] = {{0}};
    
    VkAttachmentDescription* colorAttachmentDesc = attachmentDescs + colorAttachmentIndex;
    colorAttachmentDesc->format = swapChain->format;
    colorAttachmentDesc->samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDesc->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDesc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDesc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDesc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDesc->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // layout assumed before renderpass
    colorAttachmentDesc->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // layout to automatically transition to after renderpass

    VkAttachmentReference colorAttachmentRefs[1] = {{0}};
    colorAttachmentRefs[0].attachment = colorAttachmentIndex;
    colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescs[1] = {{0}};
    subpassDescs[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescs[0].colorAttachmentCount = 1;
    // NOTE: The index of the attachments in this array are directly referenced in the fragment shader
    // ex: "layout(location = 0) out vec4 outColor" represents the color attachment ref with index 0
    subpassDescs[0].pColorAttachments = colorAttachmentRefs;

    VkSubpassDependency subpassDependencies[1] = {{0}};
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].srcAccessMask = 0;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo renderPassCI{};
    renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCI.attachmentCount = 1;
    renderPassCI.pAttachments = attachmentDescs;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = subpassDescs;
    renderPassCI.dependencyCount = 1;
    renderPassCI.pDependencies = subpassDependencies;

    if (vkCreateRenderPass(*logicalDevice, &renderPassCI, nullAllocator, renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void initFramebuffers(VkDevice* logicalDevice, SwapChain* swapChain, VkRenderPass* renderPass) {
    swapChain->framebufferCount = swapChain->imageCount;
    swapChain->framebuffers = new VkFramebuffer[swapChain->framebufferCount];

    for (uint32 i = 0; i < swapChain->framebufferCount; ++i) {
        VkFramebufferCreateInfo framebufferCI{};
        framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCI.renderPass = *renderPass;
        framebufferCI.attachmentCount = 1;
        framebufferCI.pAttachments = &swapChain->imageViews[i];
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
    VkPhysicalDevice physicalDevice;
    QueueFamilyIndices queueFamilyIndices{};
    
    vulkanContext->currentFrame = 0;
    
    initVulkanInstance(&vulkanContext->instance, &vulkanContext->debugMessenger);
    initSurface(window, &vulkanContext->instance, &vulkanContext->surface);
    pickPhysicalDevice(vulkanContext, &physicalDevice, &queueFamilyIndices);
    
    initLogicalDeviceAndQueues(&vulkanContext->logicalDevice, &physicalDevice, &queueFamilyIndices);
    vkGetDeviceQueue(vulkanContext->logicalDevice, queueFamilyIndices.graphics, 0, &vulkanContext->graphicsQueue);
    vkGetDeviceQueue(vulkanContext->logicalDevice, queueFamilyIndices.present, 0, &vulkanContext->presentQueue);
    vkGetDeviceQueue(vulkanContext->logicalDevice, queueFamilyIndices.transfer, 0, &vulkanContext->transferQueue);
    initSwapChain(&vulkanContext->surface, &physicalDevice, &vulkanContext->logicalDevice, &queueFamilyIndices, &vulkanContext->swapChain);
    initImageViews(&vulkanContext->logicalDevice, &vulkanContext->swapChain);
    initRenderPass(&vulkanContext->logicalDevice, &vulkanContext->swapChain, &vulkanContext->renderPass);
    initGraphicsPipeline(&vulkanContext->logicalDevice, &vulkanContext->swapChain.extent, &vulkanContext->pipelineLayout, &vulkanContext->renderPass, &vulkanContext->graphicsPipeline);
    initFramebuffers(&vulkanContext->logicalDevice, &vulkanContext->swapChain, &vulkanContext->renderPass);
    initCommandPool(&vulkanContext->logicalDevice, &vulkanContext->commandPool, &queueFamilyIndices);
    initCommandBuffers(&vulkanContext->logicalDevice, &vulkanContext->commandPool, &vulkanContext->commandBuffers, &vulkanContext->commandBufferCount, &vulkanContext->swapChain);
    initSyncObjects(&vulkanContext->logicalDevice, &vulkanContext->imageAvailableSemaphores, &vulkanContext->renderFinishedSemaphores, &vulkanContext->inFlightFences, &vulkanContext->imagesInFlight, &vulkanContext->swapChain);
    initVertexAttributes(vulkanContext);
}

void initVulkanInstance(VkInstance* vulkanInstance, VkDebugUtilsMessengerEXT* debugMessenger) {
    if(enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested but not found");
    }
    
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = APP_NAME;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = ENGINE_NAME;
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

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI = {};
    if(enableValidationLayers) {
        instanceCI.enabledLayerCount = ArrayCount(VALIDATION_LAYERS);
        instanceCI.ppEnabledLayerNames = VALIDATION_LAYERS;

        debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugUtilsMessengerCI.pfnUserCallback = debugCallback;
        
        instanceCI.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugUtilsMessengerCI;
    } else {
        instanceCI.enabledLayerCount = 0;
        instanceCI.pNext = nullptr;
    }
    
    if (vkCreateInstance(&instanceCI, nullAllocator, vulkanInstance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    if(enableValidationLayers) {
        if(initDebugUtilsMessengerEXT(vulkanInstance, &debugUtilsMessengerCI, nullAllocator, debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    printAvailableExtensions();

    delete[] extensions;
}

bool32 checkValidationLayerSupport() {
    uint32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        
    VkLayerProperties* availableLayers = new VkLayerProperties[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    bool32 layersFound = false;
    for (uint32 i = 0; i < ArrayCount(VALIDATION_LAYERS); i++) {
        layersFound = false;

        const char* validationLayer = VALIDATION_LAYERS[i];
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
    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT");
    if (vkCreateDebugUtilsMessengerEXT != nullptr) {
        return vkCreateDebugUtilsMessengerEXT(*instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance* instance,
                                   VkDebugUtilsMessengerEXT* debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkDestroyDebugUtilsMessengerEXT");
    if (vkDestroyDebugUtilsMessengerEXT != nullptr) {
       vkDestroyDebugUtilsMessengerEXT(*instance, *debugMessenger, pAllocator);
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

    vkDestroyBuffer(vulkanContext->logicalDevice, vulkanContext->vertices.buffer, nullAllocator);
    vkFreeMemory(vulkanContext->logicalDevice, vulkanContext->vertices.memory, nullAllocator);
    vkDestroyBuffer(vulkanContext->logicalDevice, vulkanContext->indices.buffer, nullAllocator);
    vkFreeMemory(vulkanContext->logicalDevice, vulkanContext->indices.memory, nullAllocator);
    vkDestroyRenderPass(vulkanContext->logicalDevice, vulkanContext->renderPass, nullAllocator);
    vkDestroyPipeline(vulkanContext->logicalDevice, vulkanContext->graphicsPipeline, nullAllocator);
    vkDestroyPipelineLayout(vulkanContext->logicalDevice, vulkanContext->pipelineLayout, nullAllocator);
    vkDestroySwapchainKHR(vulkanContext->logicalDevice, vulkanContext->swapChain.handle, nullAllocator);
    vkDestroySurfaceKHR(vulkanContext->instance, vulkanContext->surface, nullAllocator);
    vkDestroyDevice(vulkanContext->logicalDevice, nullAllocator);
    vkDestroyInstance(vulkanContext->instance, nullAllocator);
    
    delete[] vulkanContext->swapChain.images;
    delete[] vulkanContext->swapChain.framebuffers;
    delete[] vulkanContext->swapChain.imageViews;
    delete[] vulkanContext->commandBuffers;
    delete[] vulkanContext->renderFinishedSemaphores;
    delete[] vulkanContext->imageAvailableSemaphores;
    delete[] vulkanContext->inFlightFences;
    delete[] vulkanContext->imagesInFlight;
    
    glfwDestroyWindow(window);
    glfwTerminate();
}