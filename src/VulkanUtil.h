#pragma once

#include <vulkan/vulkan.hpp>
#include "Platform.h"
#include "FileLocations.h"
#include "Util.h"

file_access VkPipelineRasterizationStateCreateInfo defaultRasterizationCI {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // sType
        nullptr, // pNext
        0, // flags
        VK_FALSE, // depth clamp enabled
        VK_FALSE, // rasterizer discard enabled
        VK_POLYGON_MODE_FILL, // polygon mode
        VK_CULL_MODE_NONE , // cull mode // TODO: Add back face culling
        VK_FRONT_FACE_CLOCKWISE, // front face
        VK_FALSE, // depth bias enabled
        0.0f, // depth bias constant factor
        0.0f, // depth bias clamp
        0.0f, // depth bias slope factor
        1.0f // line width
};

file_access VkPipelineMultisampleStateCreateInfo defaultMultisampleCI{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, // sType
        nullptr, // pNext
        0, // flags
        VK_SAMPLE_COUNT_1_BIT, // rasterization samples
        VK_FALSE, // sample shading enabled
        1.0, // min sample shading
        nullptr, // sample mask
        VK_FALSE, // alpha to coverage enabled
        VK_FALSE // alpha to one enabled
};

file_access VkPipelineColorBlendAttachmentState defaultColorBlendAttachment{
        VK_FALSE, // blend enabled
        VK_BLEND_FACTOR_ONE, // src color blend factor
        VK_BLEND_FACTOR_ZERO, // dst color blend factor
        VK_BLEND_OP_ADD, // color blend op
        VK_BLEND_FACTOR_ONE, // src alpha blend factor
        VK_BLEND_FACTOR_ZERO, // dst alpha blend factor
        VK_BLEND_OP_ADD, // alpha blend op
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT // color write mask
};

file_access VkPipelineColorBlendStateCreateInfo defaultColorBlend{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, // sType
        nullptr, // pNext
        0, // flags
        VK_FALSE, // logical operators enabled
        VK_LOGIC_OP_COPY, // logical operator
        1, // attachment count
        &defaultColorBlendAttachment,
        { 0.0f, 0.0f, 0.0f, 0.0f } // blend constants
};

class PipelineBuilder {
public:

  PipelineBuilder(VkDevice logicalDevice, VkAllocationCallbacks* allocator = nullptr): logicalDevice(logicalDevice), allocator(allocator) {
    // TODO: implement push constants
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.pushConstantRangeCount = 0;
    pipelineLayoutCI.pPushConstantRanges = nullptr;
  }

  PipelineBuilder& setVertexShader(const char* fileLocation) {
    deallocateShader(vertexShaderModule, vertexShaderFile);
    setShader(fileLocation, VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, vertexShaderFile, vertexShaderStageCI);
    return *this;
  }

  PipelineBuilder& setFragmentShader(const char* fileLocation) {
    deallocateShader(fragmentShaderModule, fragmentShaderFile);
    setShader(fileLocation, VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, fragmentShaderFile, fragmentShaderStageCI);
    return *this;
  }

  PipelineBuilder& setRenderPass(VkRenderPass renderPass) {
    this->renderPass = renderPass;
    return *this;
  }

  PipelineBuilder& setDescriptorSetLayout(VkDescriptorSetLayout* descriptorSetLayout) {
    pipelineLayoutCI.setLayoutCount = 1; // descriptor set layouts
    pipelineLayoutCI.pSetLayouts = descriptorSetLayout; // num descriptor set layouts
    return *this;
  }

  PipelineBuilder& setViewport(float32 originX, float32 originY, float32 originZ, uint32 width, uint32 height, float32 depth) {
    viewport.x = originX;
    viewport.y = originY;
    viewport.minDepth = originZ;
    viewport.width = (float32)width;
    viewport.height = (float32)height;
    viewport.maxDepth = depth;
    return *this;
  }

  PipelineBuilder& setVertexAttributes(VertexAtt vertexAtt, uint32 bindingPoint) {
    vertexInputBindingDesc.binding = bindingPoint;
    vertexInputBindingDesc.stride = vertexAtt.strideInBytes;
    vertexInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    if(vertexInputAttDescs != nullptr) { delete[] vertexInputAttDescs; }

    vertexInputAttDescs = new VkVertexInputAttributeDescription[vertexAtt.attributeCount];
    for(uint32 i = 0; i < vertexAtt.attributeCount; ++i) {
      vertexInputAttDescs[i].binding = bindingPoint;
      vertexInputAttDescs[i].location = i;
      vertexInputAttDescs[i].format = vertexAtt.attributeFormat[i].format;
      vertexInputAttDescs[i].offset = vertexAtt.attributeFormat[i].offsetInBytes;
    }

    vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCI.vertexBindingDescriptionCount = 1;
    vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBindingDesc;
    vertexInputStateCI.vertexAttributeDescriptionCount = vertexAtt.attributeCount;
    vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttDescs;

    inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCI.topology = vertexAtt.primitiveTopology;
    inputAssemblyStateCI.primitiveRestartEnable = VK_FALSE;
    return *this;
  }

  PipelineBuilder& setScissor(int32 offsetX, int32 offsetY, uint32 width, uint32 height) {
    scissor.offset = { offsetX, offsetY};
    scissor.extent = { width, height };
    return *this;
  }

  void build(VkPipeline* outPipeline, VkPipelineLayout* outPipelineLayout) {
    if(!verifyIntegrity()) {
      throw std::runtime_error("pipeline builder is incomplete");
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageCI, fragmentShaderStageCI };

    // If no scissor, it is not an error. Simply set to the entire viewport.
    VkRect2D tmpScissor{};
    if(scissor.extent.width != 0) {
      tmpScissor = scissor;
    } else {
      tmpScissor.offset = { 0, 0 };
      tmpScissor.extent = { (uint32)viewport.width, (uint32)viewport.height };
    }

    VkPipelineViewportStateCreateInfo viewportCI{};
    viewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCI.viewportCount = 1;
    viewportCI.pViewports = &viewport;
    viewportCI.scissorCount = 1;
    viewportCI.pScissors = &tmpScissor;

    if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCI, allocator, outPipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.stageCount = ArrayCount(shaderStages);
    pipelineCI.pStages = shaderStages;
    pipelineCI.pVertexInputState = &vertexInputStateCI;
    pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
    pipelineCI.pViewportState = &viewportCI;
    pipelineCI.pRasterizationState = &defaultRasterizationCI;
    pipelineCI.pMultisampleState = &defaultMultisampleCI;
    pipelineCI.pDepthStencilState = nullptr;
    pipelineCI.pColorBlendState = &defaultColorBlend;
    pipelineCI.pDynamicState = nullptr; // Can be used to dynamically modify the viewport, scissor, line width, stencil reference, etc.
    pipelineCI.layout = *outPipelineLayout; // descriptor set layout and push constant info
    pipelineCI.renderPass = renderPass;
    pipelineCI.subpass = 0; // index of subpass in render pass where pipeline will be used
    pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCI.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCI, allocator, outPipeline) != VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline!");
    }
  }

  ~PipelineBuilder() {
    deallocateShader(vertexShaderModule, vertexShaderFile);
    deallocateShader(fragmentShaderModule, fragmentShaderFile);
    delete[] vertexInputAttDescs;
  }

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

  VkRenderPass renderPass = VK_NULL_HANDLE;

  bool verifyIntegrity() {
    bool integrity = logicalDevice != VK_NULL_HANDLE &&
            vertexShaderFile != nullptr &&
            fragmentShaderFile != nullptr &&
            viewport.width != 0 &&
            renderPass != VK_NULL_HANDLE;
    // TODO:
    return integrity;
  }

  PipelineBuilder& setShader(const char* fileLocation, VkShaderStageFlagBits shaderStageFlag, VkShaderModule& shaderModule, char*& shaderFile, VkPipelineShaderStageCreateInfo& shaderStageCreateInfo) {
    uint32 shaderSize;
    readFile(fileLocation, &shaderSize, nullptr);
    shaderFile = new char[shaderSize];
    readFile(fileLocation, &shaderSize, shaderFile);

    VkShaderModuleCreateInfo shaderModuleCI = {};
    shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCI.codeSize = shaderSize;
    shaderModuleCI.pCode = (const uint32*) shaderFile; // Note: there may be concerns with data alignment
    if(vkCreateShaderModule(logicalDevice, &shaderModuleCI, allocator, &shaderModule) != VK_SUCCESS) {
      throw std::runtime_error("failed to create shader module!");
    }

    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = shaderStageFlag;
    shaderStageCreateInfo.module = shaderModule;
    shaderStageCreateInfo.pName = "main";
    return *this;
  }

  void deallocateShader(VkShaderModule& shaderModule, char* shaderFile) {
    if(shaderFile != nullptr) {
      vkDestroyShaderModule(logicalDevice, shaderModule, allocator);
      delete[] shaderFile;
    }
    shaderFile = nullptr;
  }
};