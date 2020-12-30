/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Connor Haskins $
   ======================================================================== */

#include <vulkan/vulkan.hpp>

#include <stdexcept>
#include <iostream>

#define GLFW_INCLUDE_NONE // ensure GLFW doesn't load OpenGL headers
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include "VulkanApp.h"
#include "Platform.h"
#include "Util.h"
#include "Models.h"
#include "Input.h"
#include "FileLocations.h"
#include "VulkanUtil.h"
#include "UniformStructs.h"
#include "GraphicsPipelineBuilder.h"

#define SWAP_CHAIN_IMAGE_FORMAT VK_FORMAT_B8G8R8A8_SRGB
#define SWAP_CHAIN_IMAGE_COLOR_SPACE VK_COLOR_SPACE_SRGB_NONLINEAR_KHR

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
  VkSurfaceKHR surface;
  VkExtent2D windowExtent;
  SwapChain swapChain;
  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  VkCommandPool graphicsCommandPool;
  VkCommandPool transferCommandPool;
  uint32 commandBufferCount;
  VkCommandBuffer* commandBuffers;
  VkFence* commandBufferFences;

  struct {
    VkDescriptorPool descriptorPool;
    VkDescriptorSet* descriptorSets;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDeviceMemory memory;
    VkBuffer buffer;
    uint32* offsets;
    uint32 count;
    uint32 alignment;
  } uniformBuffers;

  struct {
    VkDevice logical;
    VkPhysicalDevice physical;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VkDeviceSize minUniformBufferOffsetAlignment;
    struct{
      VkQueue graphics;
      VkQueue present;
      VkQueue transfer;
    } queues;
  } device;

  struct{
      VkSemaphore present;
      VkSemaphore render;
  } semaphores;

  struct {
      VertexAtt info;
      VkDeviceMemory memory;
      VkBuffer buffer;
      VkDeviceSize bufferOffset;
      uint32 verticesOffset;
      uint32 indicesOffset;
  } vertexAtt;
};

void initGLFW(GLFWwindow** window, VulkanContext* vulkanContext);
void initVulkan(GLFWwindow* window, VulkanContext* vulkanContext);
bool32 checkValidationLayerSupport();
void initVulkanInstance(VkInstance* instance, VkDebugUtilsMessengerEXT* debugMessenger);
void printAvailableExtensions();
void mainLoop(GLFWwindow* window, VulkanContext* vulkanContext);
void cleanup(GLFWwindow* window, VulkanContext* vulkanContext);
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
void drawFrame(VulkanContext* vulkanContext);
void getRequiredExtensions(const char ** extensions, uint32 *extensionCount);
void processKeyboardInput();
void initFramebuffers(VulkanContext* vulkanContext);
void destroyFramebuffers(VulkanContext* vulkanContext);
void initImageViews(VkDevice* logicalDevice, SwapChain* swapChain);
void initDescriptorSetLayout(VulkanContext* vulkanContext);
void destroyImageViews(VulkanContext* vulkanContext);
void initSwapChainCommandBuffers(VulkanContext* vulkanContext);
void populateCommandBuffers(VulkanContext* vulkanContext);
void initSwapChain(VulkanContext* vulkanContext, QueueFamilyIndices queueFamilyIndices);
void initRenderPass(VkDevice* logicalDevice, VkFormat colorAttachmentFormat, VkRenderPass* renderPass);
void initUniformBuffers(VulkanContext* vulkanContext);
void initGraphicsPipeline(VulkanContext* vulkanContext);
void initCommandPools(VulkanContext* vulkanContext, QueueFamilyIndices queueFamilyIndices);
void prepareUniformBufferMemory(VulkanContext* vulkanContext);
void initDescriptorPool(VulkanContext* vulkanContext);
void initDescriptorSets(VulkanContext* vulkanContext);

const uint32 INITIAL_VIEWPORT_WIDTH = 1200;
const uint32 INITIAL_VIEWPORT_HEIGHT = 1200;

const uint32 TRANS_MATS_UNIFORM_BUFFER_BINDING_INDEX = 0;

const uint32 QUAD_VERTEX_INPUT_BINDING_INDEX = 0;
const uint64 DEFAULT_FENCE_TIMEOUT = 100000000000;

#ifdef NOT_DEBUG
bool32 enableValidationLayers = false;
#else
bool32 enableValidationLayers = true;
#endif

const char* VALIDATION_LAYERS[] = { "VK_LAYER_KHRONOS_validation" };
const char* DEVICE_EXTENSIONS[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
const char* APP_NAME = "Hello Vulkan";
const char* ENGINE_NAME = "Kuring";

const VkAllocationCallbacks* nullAllocator = nullptr;

void runVulkanApp() {
  GLFWwindow* window;
  VulkanContext vulkanContext;

  initGLFW(&window, &vulkanContext);
  initializeInput(window);
  initVulkan(window, &vulkanContext);
  mainLoop(window, &vulkanContext);
  cleanup(window, &vulkanContext);
}

void mainLoop(GLFWwindow* window, VulkanContext* vulkanContext) {
  populateCommandBuffers(vulkanContext);

  while (!glfwWindowShouldClose(window)) {
    processKeyboardInput();
    drawFrame(vulkanContext);
    glfwPollEvents();
  }

  vkDeviceWaitIdle(vulkanContext->device.logical);
}

void processKeyboardInput() {
  loadInputStateForFrame();

  if (hotPress(KeyboardInput_Esc)) {
    closeWindow();
  }

  if(isActive(KeyboardInput_Alt_Right) && hotPress(KeyboardInput_Enter)) {
    toggleWindowSize(INITIAL_VIEWPORT_WIDTH, INITIAL_VIEWPORT_HEIGHT);
  }
}

void recreateSwapChain(VulkanContext* vulkanContext)
{
  VkDevice device = vulkanContext->device.logical;
  Extent2D windowExtent = getWindowExtent();
  while (windowExtent.width == 0 || windowExtent.height == 0) {
    glfwWaitEvents();
    windowExtent = getWindowExtent();
  }

  vulkanContext->windowExtent = { windowExtent.width, windowExtent.height };

  // Ensure all operations on the device have been finished before destroying resources
  vkDeviceWaitIdle(device);

  // cleanup
  vkFreeCommandBuffers(device, vulkanContext->graphicsCommandPool, vulkanContext->commandBufferCount, vulkanContext->commandBuffers);
  destroyFramebuffers(vulkanContext);
  vkDestroyPipeline(device, vulkanContext->graphicsPipeline, nullAllocator);
  vkDestroyPipelineLayout(device, vulkanContext->pipelineLayout, nullAllocator);
  vkDestroyDescriptorPool(device, vulkanContext->uniformBuffers.descriptorPool, nullAllocator);
  vkDestroyDescriptorSetLayout(device, vulkanContext->uniformBuffers.descriptorSetLayout, nullAllocator);
  vkDestroyBuffer(device, vulkanContext->uniformBuffers.buffer, nullAllocator);
  vkFreeMemory(device, vulkanContext->uniformBuffers.memory, nullAllocator);
  destroyImageViews(vulkanContext);
  vkDestroySwapchainKHR(device, vulkanContext->swapChain.handle, nullAllocator);

  QueueFamilyIndices queueFamilyIndices;
  findQueueFamilies(vulkanContext->surface, vulkanContext->device.physical, &queueFamilyIndices);

  // Note: Recreate swap chain with new dimensions
  initSwapChain(vulkanContext, queueFamilyIndices);
  // image views are directly associated with swap chain images
  initImageViews(&device, &vulkanContext->swapChain);
  // Uniform buffer count depends on number of images in the swap chain
  prepareUniformBufferMemory(vulkanContext);
  // descriptor set count depends on swap chain image count
  initDescriptorPool(vulkanContext);
  initDescriptorSetLayout(vulkanContext);
  initDescriptorSets(vulkanContext);
  // The graphics pipeline contains viewport/scissor information that most likely needs to be updated
  initGraphicsPipeline(vulkanContext);
  // Framebuffers references the render pass and, in our situation, are wrappers around image views of the swap chain's images
  initFramebuffers(vulkanContext);
  // Command buffer count depends on swap chain image count
  initSwapChainCommandBuffers(vulkanContext);
  populateCommandBuffers(vulkanContext);

  // TODO: notify shaders uniforms
}

void updateUniformBuffer(VulkanContext* vulkanContext, uint32 index) {
  local_access auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float32 time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  TransMats ubo{};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj = glm::perspective(glm::radians(45.0f), (float32)vulkanContext->swapChain.extent.width / vulkanContext->swapChain.extent.height, 0.1f, 10.0f);
  void* uniformBufferMemoryPointer;
  vkMapMemory(vulkanContext->device.logical, vulkanContext->uniformBuffers.memory, vulkanContext->uniformBuffers.offsets[index], sizeof(ubo), 0, &uniformBufferMemoryPointer);
    memcpy(uniformBufferMemoryPointer, &ubo, sizeof(ubo));
  vkUnmapMemory(vulkanContext->device.logical, vulkanContext->uniformBuffers.memory);
}

void drawFrame(VulkanContext* vulkanContext)
{
  uint32 swapChainImageIndex; // index inside swapChain.images
  VkResult acquireResult = vkAcquireNextImageKHR(vulkanContext->device.logical,
                        vulkanContext->swapChain.handle,
                        UINT64_MAX,
                        vulkanContext->semaphores.present/*get informed when presentation is complete*/,
                        VK_NULL_HANDLE /*fence signal*/,
                        &swapChainImageIndex);

  if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain(vulkanContext);
    acquireResult = vkAcquireNextImageKHR(
            vulkanContext->device.logical,
             vulkanContext->swapChain.handle,
             UINT64_MAX,
             vulkanContext->semaphores.present/*get informed when presentation is complete*/,
             VK_NULL_HANDLE /*fence signal*/,
             &swapChainImageIndex);
  }

  if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  updateUniformBuffer(vulkanContext, swapChainImageIndex);

  vkWaitForFences(vulkanContext->device.logical, 1, &vulkanContext->commandBufferFences[swapChainImageIndex], VK_TRUE, UINT64_MAX);
  vkResetFences(vulkanContext->device.logical, 1, &vulkanContext->commandBufferFences[swapChainImageIndex]);

  VkSemaphore drawWaitSemaphores[] = { vulkanContext->semaphores.present }; // which semaphores to wait for
  VkPipelineStageFlags drawWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // what stages of the corresponding semaphores to wait for
  VkSemaphore drawSignalSemaphores[] = { vulkanContext->semaphores.render }; // which semaphores to signal when completed

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &vulkanContext->commandBuffers[swapChainImageIndex];
  submitInfo.pWaitDstStageMask = drawWaitStages;
  submitInfo.waitSemaphoreCount = ArrayCount(drawWaitSemaphores);
  submitInfo.pWaitSemaphores = drawWaitSemaphores; // Ensure that the image has been presented before we modify it
  submitInfo.signalSemaphoreCount = ArrayCount(drawSignalSemaphores);
  submitInfo.pSignalSemaphores = drawSignalSemaphores; // signal when the queues work has been completed

  if (vkQueueSubmit(vulkanContext->device.queues.graphics, 1, &submitInfo,
                    vulkanContext->commandBufferFences[swapChainImageIndex] /* signaled on completion of all submitted command buffers */
                    ) != VK_SUCCESS) {
      throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkSemaphore presentWaitSemaphores[] = { vulkanContext->semaphores.render };
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = ArrayCount(presentWaitSemaphores);
  presentInfo.pWaitSemaphores = presentWaitSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &vulkanContext->swapChain.handle;
  presentInfo.pImageIndices = &swapChainImageIndex;
  presentInfo.pResults = nullptr; // returns a list of results to determine which swap chain may have failed

  VkResult queueResult = vkQueuePresentKHR(vulkanContext->device.queues.present, &presentInfo);

  if (queueResult == VK_ERROR_OUT_OF_DATE_KHR || queueResult == VK_SUBOPTIMAL_KHR) {
    recreateSwapChain(vulkanContext);
  } else if (queueResult != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }
}

/*
 * - Populate a command buffer associated with each of the swap chain framebuffers with the following commands
 *    - Begin command buffer
 *      - Begin render pass
 *        - bind pipeline
 *        - bind vertex attribute buffer
 *        - bind index buffer
 *        - draw
 *      - End render pass
 *    - End command buffer
 */
void populateCommandBuffers(VulkanContext* vulkanContext) {
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

    VkClearValue clearValue;
    clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};
    // NOTE: clear value is a union that may also be used as... clearValue.depthStencil = {1.0f, 0.0f}
    renderPassBeginInfo.clearValueCount = 1; // we can have a clear value for each attachment
    renderPassBeginInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(vulkanContext->commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
      vkCmdBindPipeline(vulkanContext->commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanContext->graphicsPipeline);

      // Bind triangle vertex buffer (contains position and colors)
      vkCmdBindVertexBuffers(vulkanContext->commandBuffers[i],
                             QUAD_VERTEX_INPUT_BINDING_INDEX/*First binding index as described by VkVertexInputBindingDescription.binding*/,
                             1 /*Vertex buffer count*/,
                             &vulkanContext->vertexAtt.buffer /*Vertex buffer array*/,
                             &vulkanContext->vertexAtt.bufferOffset /*offset of vertex attributes in the associated buffers*/);

      // Bind triangle index buffer
      vkCmdBindIndexBuffer(vulkanContext->commandBuffers[i],
                           vulkanContext->vertexAtt.buffer,
                           quadPosColVertexAtt.sizeInBytes, // offset in buffer
                           VK_INDEX_TYPE_UINT32);

      vkCmdBindDescriptorSets(vulkanContext->commandBuffers[i],
              VK_PIPELINE_BIND_POINT_GRAPHICS,
              vulkanContext->pipelineLayout,
              0,
              1,
              &vulkanContext->uniformBuffers.descriptorSets[i],
              0,
              nullptr);

      // Draw indexed triangle
      vkCmdDrawIndexed(vulkanContext->commandBuffers[i],
              quadPosColVertexAtt.indices.count,
              1,
              0,
              0,
              1);
    }
    vkCmdEndRenderPass(vulkanContext->commandBuffers[i]);

    if (vkEndCommandBuffer(vulkanContext->commandBuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }
  }
}

/*
 * - Create logical device wrapper around the physical device
 *    - Specify special features that will be used (ex: geometry shader, tesselation shader, multi viewport, adjustable line widths)
 *    - Specify extensions (swap chain extension, in our case)
 *    - Specify layers (debug validation layer, in our case)
 *    - Create and specify queues that will be needed using the queue family indices stored when picking the physical device
 */
void initLogicalDeviceAndQueues(VkDevice* logicalDevice, VkPhysicalDevice* physicalDevice, QueueFamilyIndices* queueFamilyIndices) {
    const float32 queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCIs[2];

    uint32 uniqueQueuesCount = 1;
    VkDeviceQueueCreateInfo graphicsQueueCI{};
    graphicsQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueCI.queueFamilyIndex = queueFamilyIndices->graphics;
    graphicsQueueCI.queueCount = 1;
    graphicsQueueCI.pQueuePriorities = &queuePriority;
    queueCIs[0] = graphicsQueueCI;

    if(queueFamilyIndices->present != queueFamilyIndices->graphics) {
      VkDeviceQueueCreateInfo presentQueueCI = graphicsQueueCI;
      presentQueueCI.queueFamilyIndex = queueFamilyIndices->present;
      queueCIs[uniqueQueuesCount++] = presentQueueCI;
    }

    if(queueFamilyIndices->transfer != queueFamilyIndices->graphics &&
       queueFamilyIndices->transfer != queueFamilyIndices->present) {
      VkDeviceQueueCreateInfo transferQueueCI = graphicsQueueCI;
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
    deviceCI.ppEnabledLayerNames = VALIDATION_LAYERS;

    if(vkCreateDevice(*physicalDevice, &deviceCI, nullAllocator, logicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
}

/*
 * Get surface using GLFW and instance
 */
void initSurface(GLFWwindow* window, VkInstance* instance, VkSurfaceKHR* surface) {
    if (glfwCreateWindowSurface(*instance, window, nullptr, surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

// Callback for when screen changes size
void framebuffer_size_callback(GLFWwindow* window, int32 width, int32 height)
{
  // TODO: Do we even need this?
  //VulkanContext* vulkanContext = (VulkanContext*)glfwGetWindowUserPointer(window);
}

void initGLFW(GLFWwindow** window, VulkanContext* vulkanContext) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // We don't want an OpenGL context on creation
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  *window = glfwCreateWindow(INITIAL_VIEWPORT_WIDTH, INITIAL_VIEWPORT_HEIGHT, APP_NAME, nullptr/*which monitor*/, nullptr);
  glfwSetWindowUserPointer(*window, vulkanContext);
  glfwSetFramebufferSizeCallback(*window, framebuffer_size_callback);
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  uint32 centeringUpperLeftX = (mode->width / 2) - (INITIAL_VIEWPORT_WIDTH / 2);
  uint32 centeringUpperLeftY = (mode->height / 2) - (INITIAL_VIEWPORT_HEIGHT / 2);
  glfwSetWindowMonitor(*window, NULL/*Null for windowed mode*/, centeringUpperLeftX, centeringUpperLeftY, INITIAL_VIEWPORT_WIDTH, INITIAL_VIEWPORT_HEIGHT, GLFW_DONT_CARE);
}

bool32 checkPhysicalDeviceExtensionSupport(VkPhysicalDevice* device){
    uint32 extensionCount;
    vkEnumerateDeviceExtensionProperties(*device, nullptr, &extensionCount, nullptr);
    VkExtensionProperties* availableExtensions = new VkExtensionProperties[extensionCount];
    vkEnumerateDeviceExtensionProperties(*device, nullptr, &extensionCount, availableExtensions);

    uint32 supportedExtensionsFound = 0;
    for(uint32 desiredExtensionIndex = 0; desiredExtensionIndex < ArrayCount(DEVICE_EXTENSIONS); ++desiredExtensionIndex) {
        if(supportedExtensionsFound < desiredExtensionIndex) break;
        for(uint32 availableExtensionsIndex = 0; availableExtensionsIndex < extensionCount; ++availableExtensionsIndex) {
            if(strcmp(DEVICE_EXTENSIONS[desiredExtensionIndex], availableExtensions[availableExtensionsIndex].extensionName) == 0) {
                ++supportedExtensionsFound;
                break;
            }
        }
    }

    delete[] availableExtensions;
    return supportedExtensionsFound == ArrayCount(DEVICE_EXTENSIONS);
}

bool32 checkPhysicalDeviceSwapChainSupport(VkPhysicalDevice* physicalDevice, VkSurfaceKHR* surface) {
    uint32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(*physicalDevice, *surface, &formatCount, nullptr);

    uint32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(*physicalDevice, *surface, &presentModeCount, nullptr);

    // ensure we have any format and some present mode for use
    return formatCount > 0 && presentModeCount > 0;
}

/*
 * - Create a swap chain that is tied to our specified surface
 *    - prefer B8G8R8A8 SRGB image format
 *    - prefer mailbox present mode (presentation engine waits for the next vertical blanking)
 *    - prefer one more than the minimum image count
 *    - prefer image extents equal to that of VulkanContext.windowExtent
 *    - specify that we want single images and not array images
 *    - specify that we are going to only use the images as color attachments
 *    - specify if our images will be used by multiple queues (depends if our graphics queue is the same as our present queue)
 *  - Swap chain images are created alongside the swap chain
 */
void initSwapChain(VulkanContext* vulkanContext, QueueFamilyIndices queueFamilyIndices) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanContext->device.physical, vulkanContext->surface, &surfaceCapabilities);

    uint32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanContext->device.physical, vulkanContext->surface, &formatCount, nullptr);
    VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanContext->device.physical, vulkanContext->surface, &formatCount, surfaceFormats);

  VkSurfaceFormatKHR surfaceFormat = { SWAP_CHAIN_IMAGE_FORMAT, SWAP_CHAIN_IMAGE_COLOR_SPACE };
  bool imageFormatAndColorSpaceFound = false;
  for (uint32 i = 0; i < formatCount; ++i) {
      if (surfaceFormats[i].format == surfaceFormat.format && surfaceFormats[i].colorSpace == surfaceFormat.colorSpace) {
        imageFormatAndColorSpaceFound = true;
        break;
      }
  }
  if(!imageFormatAndColorSpaceFound) {
    throw std::runtime_error("physical device w/ surface does not contain desired image format and/or color space");
  }
  
  vulkanContext->swapChain.format = surfaceFormat.format;

    uint32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanContext->device.physical, vulkanContext->surface, &presentModeCount, nullptr);
    VkPresentModeKHR* presentModes = new VkPresentModeKHR[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanContext->device.physical, vulkanContext->surface, &presentModeCount, presentModes);

    VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR; // guaranteed to be available
    for(uint32 i = 0; i < presentModeCount; ++i) {
        if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) { // allows triple buffering
            selectedPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

  vulkanContext->swapChain.extent = surfaceCapabilities.currentExtent;
    // indication from window manager that we may have different extent dims than the resolution of the window/surface
    if (surfaceCapabilities.currentExtent.width == UINT32_MAX) {
        VkExtent2D minExtent = surfaceCapabilities.minImageExtent;
        VkExtent2D maxExtent = surfaceCapabilities.maxImageExtent;
      vulkanContext->swapChain.extent.width = clamp(minExtent.width, maxExtent.width, vulkanContext->windowExtent.width);
      vulkanContext->swapChain.extent.height = clamp(minExtent.height, maxExtent.height, vulkanContext->windowExtent.height);
    }

    uint32 imageCount = surfaceCapabilities.minImageCount + 1;
    if(surfaceCapabilities.maxImageCount != 0 // there is an image limit
       && surfaceCapabilities.maxImageCount < imageCount) { // our current count is above that limit
        imageCount = surfaceCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCI{};
    swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCI.surface = vulkanContext->surface;
    swapChainCI.minImageCount = imageCount;
    swapChainCI.imageFormat = surfaceFormat.format;
    swapChainCI.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCI.imageExtent = vulkanContext->swapChain.extent;
    swapChainCI.imageArrayLayers = 1; // 1 unless creating stereoscopic 3D application
    swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32 createFamilyIndices[] = { queueFamilyIndices.graphics, queueFamilyIndices.present };
    if(createFamilyIndices[0] != createFamilyIndices[1]) {
        swapChainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCI.queueFamilyIndexCount = 2;
    } else {
        swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCI.queueFamilyIndexCount = 1;
    }
  swapChainCI.pQueueFamilyIndices = createFamilyIndices;

    swapChainCI.preTransform = surfaceCapabilities.currentTransform;
    swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore blending with other windows on the system
    swapChainCI.presentMode = selectedPresentMode;
    swapChainCI.clipped = VK_TRUE; // ignore pixels obscured by other windows
    swapChainCI.oldSwapchain = VK_NULL_HANDLE; // used in the event that our swap chain needs to be recreated at runtime

    if (vkCreateSwapchainKHR(vulkanContext->device.logical, &swapChainCI, nullAllocator, &vulkanContext->swapChain.handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(vulkanContext->device.logical, vulkanContext->swapChain.handle, &vulkanContext->swapChain.imageCount, nullptr);
    vulkanContext->swapChain.images = new VkImage[vulkanContext->swapChain.imageCount];
    vkGetSwapchainImagesKHR(vulkanContext->device.logical, vulkanContext->swapChain.handle, &vulkanContext->swapChain.imageCount, vulkanContext->swapChain.images);

    delete[] surfaceFormats;
    delete[] presentModes;
}

/*
 * Use vulkan instance to find a pick a physical device (actual components in computer that can run Vulkan API)
 *    - Devices are picked on qualities such as:
 *      - Being a discrete GPU
 *      - Containing graphics, present, and transfer queues
 *      - Supports our desired extensions
 *      - Swap chain supports some color format can can present to our surface
 * Also sets the queue family indices associated with the physical device
 */
void pickPhysicalDevice(VulkanContext* vulkanContext) {
    VkInstance vulkanInstance = vulkanContext->instance;
    VkSurfaceKHR surface = vulkanContext->surface;

    vulkanContext->device.physical = VK_NULL_HANDLE;
    uint32 deviceCount = 0;
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);

    if(deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[deviceCount];
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, physicalDevices);
    QueueFamilyIndices queueFamilyIndices;

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    for(uint32 i = 0; i < deviceCount; ++i) {
        VkPhysicalDevice potentialDevice = physicalDevices[i];

        vkGetPhysicalDeviceProperties(potentialDevice, &deviceProperties);
        vkGetPhysicalDeviceFeatures(potentialDevice, &deviceFeatures);


        // NOTE: Can use a more complex device selection if needed
        bool32 isDeviceSuitable = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
            && findQueueFamilies(surface, physicalDevices[i], &queueFamilyIndices)
            && checkPhysicalDeviceExtensionSupport(&potentialDevice)
            && checkPhysicalDeviceSwapChainSupport(&potentialDevice, &surface);

        if(isDeviceSuitable){
            vulkanContext->device.physical = physicalDevices[i];
            break;
        }
    }

    if(vulkanContext->device.physical == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    vkGetPhysicalDeviceMemoryProperties(vulkanContext->device.physical, &vulkanContext->device.memoryProperties);
    vulkanContext->device.minUniformBufferOffsetAlignment = deviceProperties.limits.minUniformBufferOffsetAlignment;

    delete[] physicalDevices;
}

// This function is used to request a device memory type that supports all the property flags we request (e.g. device local, host visible)
// Upon success it will return the index of the memory type that fits our requested memory properties
// This is necessary as implementations can offer an arbitrary number of memory types with different
// memory properties.
// You can check http://vulkan.gpuinfo.org/ for details on different memory configurations
uint32 getMemoryTypeIndex(VkPhysicalDeviceMemoryProperties const* deviceMemoryProperties, uint32 memoryTypeBits, VkMemoryPropertyFlags properties)
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

/*
 * - Prepare vertex and index buffers for an indexed triangle list
 * - Transfer vertex and index buffers from host visible memory to device local memory
 * - Initialize vertex input and attribute binding to match the vertex shader
 * - TODO: pass in index data as well
 */
void prepareVertexAttributeMemory(VulkanContext* vulkanContext, VertexAtt vertexAtt)
{
    vulkanContext->vertexAtt.info = vertexAtt;
    vulkanContext->vertexAtt.verticesOffset = 0;
    vulkanContext->vertexAtt.indicesOffset = vertexAtt.sizeInBytes;

    VkDevice device = vulkanContext->device.logical;

    // Note: On memory management in Vulkan in general:
    //  This is a very complex topic and while it's fine for an example application to use small individual memory allocations, that is not
    //  what should be done in a real-world application, where you should allocate large chunks of memory at once instead. From that point
    //  we should ideally create one large buffer and offsets within that buffer pointing to our data. Although additional buffers
    //  may be used as needed.

    // Note: Static data like vertex and index buffer should be stored on the device memory
    //  for optimal (and fastest) access by the GPU

    // To achieve the transfer onto device memory we use "staging buffers":
    // - Create a buffer that's visible to the host (and can be mapped)
    // - Copy the data to this buffer
    // - Create another buffer that's local on the device (VRAM) with the same size
    // - Copy the data from the host to the device using a command buffer
    // - Delete the host visible (staging) buffer
    // - Use the device local buffers for rendering

    uint32 totalBufferSize = vertexAtt.sizeInBytes + vertexAtt.indices.sizeInBytes;

    // Create a host-visible buffer to copy the vertex data to (staging buffer)
    VkBufferCreateInfo hostVisibleVertexBufferInfo = {};
    hostVisibleVertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    hostVisibleVertexBufferInfo.size = totalBufferSize;
    hostVisibleVertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // Buffer is used as the copy source

    VkBuffer hostVisibleVertexBuffer;
    vkCreateBuffer(device, &hostVisibleVertexBufferInfo, nullAllocator, &hostVisibleVertexBuffer);

    VkMemoryRequirements hostVisisbleVertexAttBufferMemReqs;
    vkGetBufferMemoryRequirements(device, hostVisibleVertexBuffer, &hostVisisbleVertexAttBufferMemReqs);

    VkMemoryAllocateInfo vertexAttBufferMemAllocInfo = {};
    vertexAttBufferMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vertexAttBufferMemAllocInfo.allocationSize = hostVisisbleVertexAttBufferMemReqs.size;
    vertexAttBufferMemAllocInfo.memoryTypeIndex = getMemoryTypeIndex(&vulkanContext->device.memoryProperties, hostVisisbleVertexAttBufferMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDeviceMemory hostVisibleVertexMemory;
    vkAllocateMemory(device, &vertexAttBufferMemAllocInfo, nullAllocator, &hostVisibleVertexMemory);
    vkBindBufferMemory(device, hostVisibleVertexBuffer, hostVisibleVertexMemory, 0/*memory offset*/);

    // Map staging memory and copy to staging buffer
    char* hostVisibleMemoryCopyPtr;
    vkMapMemory(device, hostVisibleVertexMemory, vulkanContext->vertexAtt.verticesOffset, vertexAttBufferMemAllocInfo.allocationSize, 0/*flags*/, (void**)&hostVisibleMemoryCopyPtr);
      memcpy(hostVisibleMemoryCopyPtr, vertexAtt.data, vertexAtt.sizeInBytes); // copy over vertex attribute data
      memcpy(hostVisibleMemoryCopyPtr + vulkanContext->vertexAtt.indicesOffset, vertexAtt.indices.data, vertexAtt.indices.sizeInBytes); // copy over index data
      hostVisibleMemoryCopyPtr = nullptr;
    vkUnmapMemory(device, hostVisibleVertexMemory);

    // Create destination buffer with device only visibility
    VkBufferCreateInfo deviceLocalVertexBufferInfo = {};
    deviceLocalVertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    deviceLocalVertexBufferInfo.size = totalBufferSize;
    deviceLocalVertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; // device local buffer to be used for rendering

    vkCreateBuffer(device, &deviceLocalVertexBufferInfo, nullAllocator, &vulkanContext->vertexAtt.buffer);
    vulkanContext->vertexAtt.bufferOffset = 0;
    VkMemoryRequirements deviceLocalVertexAttBufferMemReqs;
    vkGetBufferMemoryRequirements(device, vulkanContext->vertexAtt.buffer, &deviceLocalVertexAttBufferMemReqs);
    vertexAttBufferMemAllocInfo.allocationSize = deviceLocalVertexAttBufferMemReqs.size;
    vertexAttBufferMemAllocInfo.memoryTypeIndex = getMemoryTypeIndex(&vulkanContext->device.memoryProperties, deviceLocalVertexAttBufferMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vkAllocateMemory(device, &vertexAttBufferMemAllocInfo, nullAllocator, &vulkanContext->vertexAtt.memory);
    vkBindBufferMemory(device, vulkanContext->vertexAtt.buffer, vulkanContext->vertexAtt.memory, 0);

    // Buffer copies have to be submitted to a queue, so we need a command buffer
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = vulkanContext->transferCommandPool;
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
      // Put buffer region copies into command buffer
      VkBufferCopy copyRegion;
      copyRegion.srcOffset = 0;
      copyRegion.dstOffset = 0;
      // Vertex buffer
      copyRegion.size = totalBufferSize;
      vkCmdCopyBuffer(copyCommandBuffer, hostVisibleVertexBuffer, vulkanContext->vertexAtt.buffer, 1/* region count */, &copyRegion);
    vkEndCommandBuffer(copyCommandBuffer);

    VkSubmitInfo copyCommandSubmitInfo = {};
    copyCommandSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    copyCommandSubmitInfo.commandBufferCount = 1;
    copyCommandSubmitInfo.pCommandBuffers = &copyCommandBuffer;

    // Create fence to wait for transfer to complete
    VkFenceCreateInfo copyFenceCreateInfo = {};
    copyFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    copyFenceCreateInfo.flags = 0;

    VkFence copyFence;
    vkCreateFence(device, &copyFenceCreateInfo, nullAllocator, &copyFence);

    // Submit to the queue
    vkQueueSubmit(vulkanContext->device.queues.present, 1, &copyCommandSubmitInfo, copyFence);
    // Wait for the fence to signal that command buffer has finished executing
    vkWaitForFences(device, 1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);

    vkDestroyFence(device, copyFence, nullAllocator);
    vkFreeCommandBuffers(device, vulkanContext->transferCommandPool, 1, &copyCommandBuffer);

    // Destroy staging buffers
    // Note: Staging buffer must not be deleted before the copies have been submitted and executed
    vkDestroyBuffer(device, hostVisibleVertexBuffer, nullAllocator);
    vkFreeMemory(device, hostVisibleVertexMemory, nullAllocator);
}

/*
 * - Create semaphores to wait between queue commands & command buffers
 * - Create fences that will be used to wait for completion of submitted command buffers
 */
void initSyncObjects(VulkanContext* vulkanContext) {
    vulkanContext->commandBufferFences = new VkFence[vulkanContext->commandBufferCount];

    VkSemaphoreCreateInfo semaphoreCI{};
    semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCI{};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if(vkCreateSemaphore(vulkanContext->device.logical, &semaphoreCI, nullAllocator, &vulkanContext->semaphores.present) != VK_SUCCESS ||
       vkCreateSemaphore(vulkanContext->device.logical, &semaphoreCI, nullAllocator, &vulkanContext->semaphores.render) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
    }
    for(uint32 i = 0; i < vulkanContext->commandBufferCount; ++i) {
        if (vkCreateFence(vulkanContext->device.logical, &fenceCI, nullAllocator, vulkanContext->commandBufferFences + i) != VK_SUCCESS) {
            throw std::runtime_error("failed to create fences!");
        }
    }
}

void initDescriptorSetLayout(VulkanContext* vulkanContext) {
  VkDescriptorSetLayoutBinding transMatsDescriptorSetLayoutBinding{};
  transMatsDescriptorSetLayoutBinding.binding = TRANS_MATS_UNIFORM_BUFFER_BINDING_INDEX;
  transMatsDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  transMatsDescriptorSetLayoutBinding.descriptorCount = 1;
  transMatsDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  transMatsDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &transMatsDescriptorSetLayoutBinding;

  if (vkCreateDescriptorSetLayout(vulkanContext->device.logical, &layoutInfo, nullAllocator, &vulkanContext->uniformBuffers.descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void prepareUniformBufferMemory(VulkanContext* vulkanContext) {
  vulkanContext->uniformBuffers.count = vulkanContext->swapChain.imageCount;
  uint32 uniformBufferDataSize = sizeof(TransMats);
  uint32 totalBufferSize = uniformBufferDataSize * vulkanContext->uniformBuffers.count;

  vulkanContext->uniformBuffers.offsets = new uint32[vulkanContext->uniformBuffers.count];

  // Create a host-visible buffer to copy the vertex data to (staging buffer)
  VkBufferCreateInfo uniformBufferBufferInfo = {};
  uniformBufferBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  uniformBufferBufferInfo.size = totalBufferSize;
  uniformBufferBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; // Buffer is used as the copy source

  vkCreateBuffer(vulkanContext->device.logical, &uniformBufferBufferInfo, nullAllocator, &vulkanContext->uniformBuffers.buffer);

  VkMemoryRequirements uniformBufferMemReqs;
  vkGetBufferMemoryRequirements(vulkanContext->device.logical, vulkanContext->uniformBuffers.buffer, &uniformBufferMemReqs);

  vulkanContext->uniformBuffers.alignment = (uint32)max(uniformBufferMemReqs.alignment, vulkanContext->device.minUniformBufferOffsetAlignment);
  uint32 alignmentsPerData = (uniformBufferDataSize + vulkanContext->uniformBuffers.alignment - 1) / vulkanContext->uniformBuffers.alignment;
  uint32 offsetInterval = alignmentsPerData * vulkanContext->uniformBuffers.alignment;
  uniformBufferBufferInfo.size = offsetInterval * vulkanContext->uniformBuffers.count;
  vkDestroyBuffer(vulkanContext->device.logical, vulkanContext->uniformBuffers.buffer, nullAllocator);
  vkCreateBuffer(vulkanContext->device.logical, &uniformBufferBufferInfo, nullAllocator, &vulkanContext->uniformBuffers.buffer);
  vkGetBufferMemoryRequirements(vulkanContext->device.logical, vulkanContext->uniformBuffers.buffer, &uniformBufferMemReqs);

  VkMemoryAllocateInfo uniformBufferMemAllocInfo = {};
  uniformBufferMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  uniformBufferMemAllocInfo.allocationSize = uniformBufferMemReqs.size;
  uniformBufferMemAllocInfo.memoryTypeIndex = getMemoryTypeIndex(&vulkanContext->device.memoryProperties, uniformBufferMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  vkAllocateMemory(vulkanContext->device.logical, &uniformBufferMemAllocInfo, nullAllocator, &vulkanContext->uniformBuffers.memory);
  vkBindBufferMemory(vulkanContext->device.logical, vulkanContext->uniformBuffers.buffer, vulkanContext->uniformBuffers.memory, 0/*memory offset*/);

  for(uint32 i = 0; i < vulkanContext->uniformBuffers.count; ++i) {
    vulkanContext->uniformBuffers.offsets[i] = i * offsetInterval;
  }
}

void initVulkan(GLFWwindow* window, VulkanContext* vulkanContext) {
    vulkanContext->windowExtent = { INITIAL_VIEWPORT_WIDTH, INITIAL_VIEWPORT_HEIGHT };

    initVulkanInstance(&vulkanContext->instance, &vulkanContext->debugMessenger);
    initSurface(window, &vulkanContext->instance, &vulkanContext->surface);
    pickPhysicalDevice(vulkanContext);

    QueueFamilyIndices queueFamilyIndices;
    findQueueFamilies(vulkanContext->surface, vulkanContext->device.physical, &queueFamilyIndices);

    initLogicalDeviceAndQueues(&vulkanContext->device.logical, &vulkanContext->device.physical, &queueFamilyIndices);
    vkGetDeviceQueue(vulkanContext->device.logical, queueFamilyIndices.graphics, 0, &vulkanContext->device.queues.graphics);
    vkGetDeviceQueue(vulkanContext->device.logical, queueFamilyIndices.present, 0, &vulkanContext->device.queues.present);
    vkGetDeviceQueue(vulkanContext->device.logical, queueFamilyIndices.transfer, 0, &vulkanContext->device.queues.transfer);

    initRenderPass(&vulkanContext->device.logical, SWAP_CHAIN_IMAGE_FORMAT, &vulkanContext->renderPass);
    initSwapChain(vulkanContext, queueFamilyIndices);
    initCommandPools(vulkanContext, queueFamilyIndices);
    initSwapChainCommandBuffers(vulkanContext);
    prepareVertexAttributeMemory(vulkanContext, quadPosColVertexAtt);
    prepareUniformBufferMemory(vulkanContext);
    initDescriptorSetLayout(vulkanContext);
    initDescriptorPool(vulkanContext);
    initDescriptorSets(vulkanContext);
    initImageViews(&vulkanContext->device.logical, &vulkanContext->swapChain);
    initGraphicsPipeline(vulkanContext);
    initFramebuffers(vulkanContext);
    initSyncObjects(vulkanContext);
}

void initDescriptorPool(VulkanContext* vulkanContext)
{
  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = vulkanContext->uniformBuffers.count;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = vulkanContext->uniformBuffers.count;

  if (vkCreateDescriptorPool(vulkanContext->device.logical, &poolInfo, nullptr, &vulkanContext->uniformBuffers.descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void initDescriptorSets(VulkanContext* vulkanContext) {
  std::vector<VkDescriptorSetLayout> layouts(vulkanContext->uniformBuffers.count, vulkanContext->uniformBuffers.descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = vulkanContext->uniformBuffers.descriptorPool;
  allocInfo.descriptorSetCount = vulkanContext->uniformBuffers.count;
  allocInfo.pSetLayouts = layouts.data();

  vulkanContext->uniformBuffers.descriptorSets = new VkDescriptorSet[vulkanContext->uniformBuffers.count];
  if (vkAllocateDescriptorSets(vulkanContext->device.logical, &allocInfo, vulkanContext->uniformBuffers.descriptorSets) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = vulkanContext->uniformBuffers.buffer;
  for (size_t i = 0; i < vulkanContext->uniformBuffers.count; ++i) {
    bufferInfo.offset = vulkanContext->uniformBuffers.offsets[i];
    bufferInfo.range = vulkanContext->uniformBuffers.alignment;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = vulkanContext->uniformBuffers.descriptorSets[i];
    descriptorWrite.dstBinding = TRANS_MATS_UNIFORM_BUFFER_BINDING_INDEX;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo; // used for descriptors that refer to buffer data
    descriptorWrite.pImageInfo = nullptr; // used for descriptors that refer to image data
    descriptorWrite.pTexelBufferView = nullptr; // used for descriptors that refer to buffer views

    vkUpdateDescriptorSets(vulkanContext->device.logical, 1, &descriptorWrite, 0, nullptr);
  }
}

/*
 * TLDR:
 * - Sets app name/version, engine name/version
 * - Specify vulkan API version
 * - Specify extensions: GLFW extension, debug utils extension
 * - Specify layers: Debug validation layer
 */
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
    appInfo.apiVersion = VK_MAKE_VERSION(1, 2, 0);

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
        debugUtilsMessengerCI.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
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

    if(enableValidationLayers && debugMessenger != nullptr) {
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

void destroyImageViews(VulkanContext* vulkanContext) {
  for(uint32 i = 0; i < vulkanContext->swapChain.imageCount; ++i) {
    vkDestroyImageView(vulkanContext->device.logical, vulkanContext->swapChain.imageViews[i], nullAllocator);
  }
}

void cleanup(GLFWwindow* window, VulkanContext* vulkanContext) {
  deinitializeInput();

  VkDevice device = vulkanContext->device.logical;

    if(enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(&vulkanContext->instance, &vulkanContext->debugMessenger, nullAllocator);
    }

    // NOTE: No need to call vkFreeCommandBuffers as they get freed when the command pool is destroyed
    vkDestroyCommandPool(device, vulkanContext->graphicsCommandPool, nullAllocator);
    if(vulkanContext->graphicsCommandPool != vulkanContext->transferCommandPool) {
      vkDestroyCommandPool(device, vulkanContext->transferCommandPool, nullAllocator);
    }

    destroyFramebuffers(vulkanContext);
    destroyImageViews(vulkanContext);

    for(uint32 i = 0; i < vulkanContext->commandBufferCount; ++i) {
        vkDestroyFence(device, vulkanContext->commandBufferFences[i], nullAllocator);
    }
    vkDestroySemaphore(device, vulkanContext->semaphores.render, nullAllocator);
    vkDestroySemaphore(device, vulkanContext->semaphores.present, nullAllocator);

    vkDestroyBuffer(device, vulkanContext->uniformBuffers.buffer, nullAllocator);
    vkFreeMemory(device, vulkanContext->uniformBuffers.memory, nullAllocator);
    vkDestroyDescriptorPool(device, vulkanContext->uniformBuffers.descriptorPool, nullAllocator);
    vkDestroyDescriptorSetLayout(device, vulkanContext->uniformBuffers.descriptorSetLayout, nullAllocator);
    vkDestroyBuffer(device, vulkanContext->vertexAtt.buffer, nullAllocator);
    vkFreeMemory(device, vulkanContext->vertexAtt.memory, nullAllocator);
    vkDestroyRenderPass(device, vulkanContext->renderPass, nullAllocator);
    vkDestroyPipeline(device, vulkanContext->graphicsPipeline, nullAllocator);
    vkDestroyPipelineLayout(device, vulkanContext->pipelineLayout, nullAllocator);
    vkDestroySwapchainKHR(device, vulkanContext->swapChain.handle, nullAllocator);
    vkDestroySurfaceKHR(vulkanContext->instance, vulkanContext->surface, nullAllocator);
    vkDestroyDevice(device, nullAllocator);
    vkDestroyInstance(vulkanContext->instance, nullAllocator);
    
    delete[] vulkanContext->swapChain.images;
    delete[] vulkanContext->swapChain.framebuffers;
    delete[] vulkanContext->swapChain.imageViews;
    delete[] vulkanContext->commandBuffers;
    delete[] vulkanContext->commandBufferFences;
    delete[] vulkanContext->uniformBuffers.offsets;
    delete[] vulkanContext->uniformBuffers.descriptorSets;
    
    glfwDestroyWindow(window);
    glfwTerminate();
}

void destroyFramebuffers(VulkanContext* vulkanContext)
{
  for(uint32 i = 0; i < vulkanContext->swapChain.framebufferCount; ++i) {
    vkDestroyFramebuffer(vulkanContext->device.logical, vulkanContext->swapChain.framebuffers[i], nullAllocator);
  }
}

/*
 * - Create framebuffers that are linked to the swap chain's images (through image views)
 */
void initFramebuffers(VulkanContext* vulkanContext)
{
  SwapChain* swapChain = &vulkanContext->swapChain;
  swapChain->framebufferCount = swapChain->imageCount;
  swapChain->framebuffers = new VkFramebuffer[swapChain->framebufferCount];

  for (uint32 i = 0; i < swapChain->framebufferCount; ++i) {
    VkFramebufferCreateInfo framebufferCI{};
    framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCI.renderPass = vulkanContext->renderPass;
    framebufferCI.attachmentCount = 1;
    framebufferCI.pAttachments = &swapChain->imageViews[i];
    framebufferCI.width = swapChain->extent.width;
    framebufferCI.height = swapChain->extent.height;
    framebufferCI.layers = 1;

    if (vkCreateFramebuffer(vulkanContext->device.logical, &framebufferCI, nullAllocator, &swapChain->framebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

/*
 * - Allocate primary command buffers from the VulkanContext.graphicsCommandPool to be used along size the swap chain's framebuffers
 */
void initSwapChainCommandBuffers(VulkanContext* vulkanContext)
{
  vulkanContext->commandBufferCount = vulkanContext->swapChain.imageCount;
  vulkanContext->commandBuffers = new VkCommandBuffer[vulkanContext->commandBufferCount];

  VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
  commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocateInfo.commandPool = vulkanContext->graphicsCommandPool;
  // Primary level allows us to submit directly to a queue for execution, secondary level allows us to reference from another command buffer
  commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  commandBufferAllocateInfo.commandBufferCount = vulkanContext->commandBufferCount;

  if (vkAllocateCommandBuffers(vulkanContext->device.logical, &commandBufferAllocateInfo, vulkanContext->commandBuffers) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

/*
 * - Create image views that allow us to access swap chain images
 * - Specify that we want to use hem as 2D images
 * - Image view format is as specified when creating the swap chain
 */
void initImageViews(VkDevice* logicalDevice, SwapChain* swapChain)
{
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

/*
 * Create a render pass by...
 *    - describing the attachments (in our case only 1 color attachment)
 *    - specifying dependencies between render passes
 *    - specifying what we expect to happen before we access the attachments and how we should wait for them
 *    - specifying what we expect to happen after we finish accessing the attachments and how others should wait for our renderpass
 */
void initRenderPass(VkDevice* logicalDevice, VkFormat colorAttachmentFormat, VkRenderPass* renderPass)
{
  const uint32 colorAttachmentIndex = 0;
  VkAttachmentDescription attachmentDescs[1];

  attachmentDescs[colorAttachmentIndex].format = colorAttachmentFormat;
  attachmentDescs[colorAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescs[colorAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachmentDescs[colorAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescs[colorAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentDescs[colorAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescs[colorAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // layout assumed before render pass
  attachmentDescs[colorAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // layout to automatically transition to after render pass
  attachmentDescs[colorAttachmentIndex].flags = 0;

  VkAttachmentReference colorAttachmentRefs[1];
  colorAttachmentRefs[0].attachment = colorAttachmentIndex;
  colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpassDescs[1];
  subpassDescs[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescs[0].colorAttachmentCount = 1;
  // NOTE: The index of the attachments in this array are directly referenced in the fragment shader
  // ex: "layout(location = 0) out vec4 outColor" represents the color attachment ref with index 0
  subpassDescs[0].pColorAttachments = colorAttachmentRefs;
  subpassDescs[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescs[0].inputAttachmentCount = 0;
  subpassDescs[0].pInputAttachments = nullptr;
  subpassDescs[0].preserveAttachmentCount = 0;
  subpassDescs[0].pPreserveAttachments = nullptr;
  subpassDescs[0].pDepthStencilAttachment = nullptr;
  subpassDescs[0].pResolveAttachments = nullptr;
  subpassDescs[0].flags = 0;

  VkSubpassDependency subpassDependencies[1];
  subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL; // no specified subpass before
  subpassDependencies[0].dstSubpass = 0; // no specified subpass after
  subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // We must wait for color attachment to be freed up (may be in use by swap chain)
  subpassDependencies[0].srcAccessMask = 0; // We do not care why it was being accessed previous to our uses
  subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // We should be waited on if we are using the color attachment
  subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // and specifically writing to it
  subpassDependencies[0].dependencyFlags = 0;

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

/*
 * - Read in vertex/fragment shader files and create associated shader modules
 * - Creates vertex/fragment pipeline shader stages based on shader modules
 * - Specify vertex attributes to be used as the pipeline's vertex input
 *    - input rate, stride, binding point(used in vkCmdBindVertexBuffers), location, format, offset
 * - Specify topology of vertex data [ex: triangle list, triangle strip, points, lines]
 * - Specify the viewport extent w/ offset (which defines NDCs) and scissor extent w/ offset (which defines the portion
 *                                                                           of the viewport that is actually drawn to)
 * - Specify various state like line width, fill mode, cull mode, winding order
 * - Specify multi-sampling (only 1 sample in our case)
 * - Specify color & alpha blending between draw calls
 * - Create a pipeline with all of the above + render pass(es)
 * - TODO: Specify descriptor set layouts and push constants
 */
void initGraphicsPipeline(VulkanContext* vulkanContext)
{
  GraphicsPipelineBuilder(vulkanContext->device.logical)
          .setVertexShader(POS_COLOR_TRANS_MATS_VERT_SHADER_FILE_LOC)
          .setFragmentShader(VERTEX_COLOR_FRAG_SHADER_FILE_LOC)
          .setVertexAttributes(quadPosColVertexAtt, QUAD_VERTEX_INPUT_BINDING_INDEX)
          .setDescriptorSetLayouts(&vulkanContext->uniformBuffers.descriptorSetLayout, 1)
          .setViewport(0.0, 0.0, 0.0, vulkanContext->windowExtent.width, vulkanContext->windowExtent.height, 1.0)
          .setScissor(0, 0, vulkanContext->windowExtent.width, vulkanContext->windowExtent.height)
          .setRenderPass(vulkanContext->renderPass)
          .build(&vulkanContext->graphicsPipeline, &vulkanContext->pipelineLayout);
}

/*
 * Create a command pool that specifically utilizes our discovered specified graphics queue
 * NOTE: This command pool may not have present or transfer capabilities
 */
void initCommandPools(VulkanContext* vulkanContext, QueueFamilyIndices queueFamilyIndices)
{
  VkCommandPoolCreateInfo commandPoolCI{};
  commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolCI.queueFamilyIndex = queueFamilyIndices.graphics;
  commandPoolCI.flags = 0;

  if (vkCreateCommandPool(vulkanContext->device.logical, &commandPoolCI, nullAllocator, &vulkanContext->graphicsCommandPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics command pool!");
  }

  // TODO: transfer command pool is atually unnecessary as any queue family with VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT
  //  implicitly support VK_QUEUE_TRANSFER_BIT operations. Remove transferCommandPool.
  if(queueFamilyIndices.present == queueFamilyIndices.graphics) {
    vulkanContext->transferCommandPool = vulkanContext->graphicsCommandPool;
    return;
  }

  commandPoolCI.queueFamilyIndex = queueFamilyIndices.present;
  commandPoolCI.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; // transfer commands are expected to be short lived
  if (vkCreateCommandPool(vulkanContext->device.logical, &commandPoolCI, nullAllocator, &vulkanContext->transferCommandPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}
