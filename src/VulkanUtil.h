#pragma once

#include <vulkan/vulkan_core.h>
#include "KuringTypes.h"

struct QueueFamilyIndices {
  u32 graphics;
  u32 present;
  u32 transfer;
};

bool32 findQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, QueueFamilyIndices* queueFamilyIndices);