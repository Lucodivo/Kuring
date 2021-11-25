#include "VulkanUtil.h"

bool32 findQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, QueueFamilyIndices* queueFamilyIndices) {
  u32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

  VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

  bool32 present = false;
  bool32 graphics = false;
  bool32 transfer = false;
  for (u32 i = 0; i < queueFamilyCount; ++i) {
    if(!present) {
      VkBool32 supportsPresentation = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresentation);
      if (supportsPresentation) {
        present = true;
        queueFamilyIndices->present = i;
      }
    }

    if (!graphics && (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      queueFamilyIndices->graphics = i;
      graphics = true;
    }

    if (!transfer && (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)) {
      queueFamilyIndices->transfer = i;
      transfer = true;
    }

    if(present && graphics && transfer) { break; }
  }

  delete[] queueFamilies;
  return present && graphics && transfer;
}