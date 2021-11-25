#pragma once

#include <vulkan/vulkan.hpp>
#include "KuringTypes.h"
#include "Util.h"
#include "Models.h"

class GraphicsPipelineBuilder {
public:

  GraphicsPipelineBuilder(VkDevice logicalDevice, VkAllocationCallbacks* allocator = nullptr);
  ~GraphicsPipelineBuilder();

  GraphicsPipelineBuilder& setVertexShader(const char* fileLocation);
  GraphicsPipelineBuilder& setFragmentShader(const char* fileLocation);
  GraphicsPipelineBuilder& setRenderPass(VkRenderPass renderPass);
  GraphicsPipelineBuilder& setPolygonMode(VkPolygonMode polygonMode);
  GraphicsPipelineBuilder& setCullMode(VkCullModeFlags cullModeFlags);
  GraphicsPipelineBuilder& setFrontFace(VkFrontFace frontFace);
  GraphicsPipelineBuilder& setDescriptorSetLayouts(VkDescriptorSetLayout* descriptorSetLayout, u32 count);
  GraphicsPipelineBuilder& setViewport(f32 originX, f32 originY, f32 originZ, u32 width, u32 height, f32 depth);
  GraphicsPipelineBuilder& setVertexAttributes(VertexAtt vertexAtt, u32 bindingPoint);
  GraphicsPipelineBuilder& setScissor(s32 offsetX, s32 offsetY, u32 width, u32 height);

  void build(VkPipeline* outPipeline, VkPipelineLayout* outPipelineLayout);

private:
  VkAllocationCallbacks* allocator = nullptr;
  VkDevice logicalDevice = VK_NULL_HANDLE;

  VkPipelineShaderStageCreateInfo vertexShaderStageCI{};
  VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
  VkPipelineShaderStageCreateInfo fragmentShaderStageCI{};
  VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;

  VkVertexInputBindingDescription vertexInputBindingDesc{};
  VkVertexInputAttributeDescription* vertexInputAttDescs = nullptr;
  VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
  VkPipelineLayoutCreateInfo pipelineLayoutCI{};

  VkViewport viewport{};
  VkRect2D scissor{};

  VkPolygonMode polygonMode;
  VkCullModeFlags cullMode;
  VkFrontFace frontFace;

  VkRenderPass renderPass = VK_NULL_HANDLE;

  void verifyIntegrity();
  GraphicsPipelineBuilder& setShader(const char* fileLocation, VkShaderStageFlagBits shaderStageFlag, VkShaderModule& shaderModule, VkPipelineShaderStageCreateInfo& shaderStageCreateInfo);
  void deallocateShader(VkShaderModule& shaderModule);
};
