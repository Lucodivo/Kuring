#pragma once

#include <vulkan/vulkan_core.h>
#include "Platform.h"

struct QueueFamilyIndices {
  uint32 graphics;
  uint32 present;
  uint32 transfer;
};

bool32 findQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, QueueFamilyIndices* queueFamilyIndices);