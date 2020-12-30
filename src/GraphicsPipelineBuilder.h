#pragma once

#include <vulkan/vulkan.hpp>
#include "Platform.h"
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
  GraphicsPipelineBuilder& setDescriptorSetLayouts(VkDescriptorSetLayout* descriptorSetLayout, uint32 count);
  GraphicsPipelineBuilder& setViewport(float32 originX, float32 originY, float32 originZ, uint32 width, uint32 height, float32 depth);
  GraphicsPipelineBuilder& setVertexAttributes(VertexAtt vertexAtt, uint32 bindingPoint);
  GraphicsPipelineBuilder& setScissor(int32 offsetX, int32 offsetY, uint32 width, uint32 height);

  void build(VkPipeline* outPipeline, VkPipelineLayout* outPipelineLayout);

private:
  VkAllocationCallbacks* allocator = nullptr;
  VkDevice logicalDevice = VK_NULL_HANDLE;

  VkPipelineShaderStageCreateInfo vertexShaderStageCI{};
  VkShaderModule vertexShaderModule{};
  char* vertexShaderFile = nullptr;
  VkPipelineShaderStageCreateInfo fragmentShaderStageCI{};
  VkShaderModule fragmentShaderModule{};
  char* fragmentShaderFile = nullptr;

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
  GraphicsPipelineBuilder& setShader(const char* fileLocation, VkShaderStageFlagBits shaderStageFlag, VkShaderModule& shaderModule, char*& shaderFile, VkPipelineShaderStageCreateInfo& shaderStageCreateInfo);
  void deallocateShader(VkShaderModule& shaderModule, char* shaderFile);
};
