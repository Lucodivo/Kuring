#include "GraphicsPipelineBuilder.h"

internal_access VkPipelineRasterizationStateCreateInfo defaultRasterizationCI {
VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // sType
nullptr, // pNext
0, // flags
VK_FALSE, // depth clamp enabled
VK_FALSE, // rasterizer discard enabled
VK_POLYGON_MODE_FILL, // polygon mode
VK_CULL_MODE_BACK_BIT , // cull mode
VK_FRONT_FACE_CLOCKWISE, // front face
VK_FALSE, // depth bias enabled
0.0f, // depth bias constant factor
0.0f, // depth bias clamp
0.0f, // depth bias slope factor
1.0f // line width
};

internal_access VkPipelineMultisampleStateCreateInfo defaultMultisampleCI{
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

internal_access VkPipelineColorBlendAttachmentState defaultColorBlendAttachment{
VK_FALSE, // blend enabled
VK_BLEND_FACTOR_ONE, // src color blend factor
VK_BLEND_FACTOR_ZERO, // dst color blend factor
VK_BLEND_OP_ADD, // color blend op
VK_BLEND_FACTOR_ONE, // src alpha blend factor
VK_BLEND_FACTOR_ZERO, // dst alpha blend factor
VK_BLEND_OP_ADD, // alpha blend op
VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT // color write mask
};

internal_access VkPipelineColorBlendStateCreateInfo defaultColorBlend{
VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, // sType
nullptr, // pNext
0, // flags
VK_FALSE, // logical operators enabled
VK_LOGIC_OP_COPY, // logical operator
1, // attachment count
&defaultColorBlendAttachment,
{ 0.0f, 0.0f, 0.0f, 0.0f } // blend constants
};

GraphicsPipelineBuilder::GraphicsPipelineBuilder(VkDevice logicalDevice, VkAllocationCallbacks* allocator) : logicalDevice(logicalDevice), allocator(allocator) {
  // TODO: push constants
  pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCI.pushConstantRangeCount = 0;
  pipelineLayoutCI.pPushConstantRanges = nullptr;
  pipelineLayoutCI.setLayoutCount = 0;
  pipelineLayoutCI.pSetLayouts = nullptr;

  polygonMode = defaultRasterizationCI.polygonMode;
  cullMode = defaultRasterizationCI.cullMode;
  frontFace = defaultRasterizationCI.frontFace;
}

GraphicsPipelineBuilder::~GraphicsPipelineBuilder()
{
  deallocateShader(vertexShaderModule);
  deallocateShader(fragmentShaderModule);
  delete[] vertexInputAttDescs;
}

void GraphicsPipelineBuilder::build(VkPipeline* outPipeline, VkPipelineLayout* outPipelineLayout)
{
  verifyIntegrity();

  VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageCI, fragmentShaderStageCI };

  // If no scissor, it is not an error. Simply set to the entire viewport.
  VkRect2D tmpScissor{};
  if(scissor.extent.width != 0) {
    tmpScissor = scissor;
  } else {
    tmpScissor.offset = { 0, 0 };
    tmpScissor.extent = {(u32)viewport.width, (u32)viewport.height };
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

  VkPipelineRasterizationStateCreateInfo rasterizationCI = defaultRasterizationCI;
  rasterizationCI.polygonMode = polygonMode;
  rasterizationCI.cullMode = cullMode;
  rasterizationCI.frontFace = frontFace;

  VkGraphicsPipelineCreateInfo pipelineCI{};
  pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCI.stageCount = ArrayCount(shaderStages);
  pipelineCI.pStages = shaderStages;
  pipelineCI.pVertexInputState = &vertexInputStateCI;
  pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
  pipelineCI.pViewportState = &viewportCI;
  pipelineCI.pRasterizationState = &rasterizationCI;
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


GraphicsPipelineBuilder& GraphicsPipelineBuilder::setVertexShader(const char* fileLocation)
{
  deallocateShader(vertexShaderModule);
  setShader(fileLocation, VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, vertexShaderStageCI);
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setFragmentShader(const char* fileLocation)
{
  deallocateShader(fragmentShaderModule);
  setShader(fileLocation, VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, fragmentShaderStageCI);
  return *this;
}

void GraphicsPipelineBuilder::deallocateShader(VkShaderModule& shaderModule)
{
  if(shaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(logicalDevice, shaderModule, allocator);
    shaderModule = VK_NULL_HANDLE;
  }
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setShader(const char* fileLocation, VkShaderStageFlagBits shaderStageFlag,
                                                            VkShaderModule& shaderModule,
                                                            VkPipelineShaderStageCreateInfo& shaderStageCreateInfo)
{
  u32 shaderSize;
  readFile(fileLocation, &shaderSize, nullptr);
  char* shaderFile = new char[shaderSize];
  readFile(fileLocation, &shaderSize, shaderFile);

  VkShaderModuleCreateInfo shaderModuleCI = {};
  shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderModuleCI.codeSize = shaderSize;
  shaderModuleCI.pCode = (const u32*) shaderFile; // Note: there may be concerns with data alignment
  if(vkCreateShaderModule(logicalDevice, &shaderModuleCI, allocator, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageCreateInfo.stage = shaderStageFlag;
  shaderStageCreateInfo.module = shaderModule;
  shaderStageCreateInfo.pName = "main";
  delete[] shaderFile;
  return *this;
}

void GraphicsPipelineBuilder::verifyIntegrity()
{
  const std::string errorTitle = "GraphicsPipelineBuilder error: ";

  if(logicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error(errorTitle + "failed to supply logical device!");
  }
  if(renderPass == VK_NULL_HANDLE) {
    throw std::runtime_error(errorTitle + "failed to supply render pass!");
  }
  if(vertexInputAttDescs == nullptr) {
    throw std::runtime_error(errorTitle + "failed to supply vertex attributes!");
  }
  if(vertexShaderModule == VK_NULL_HANDLE) {
    throw std::runtime_error(errorTitle + "failed to supply vertex shader!");
  }
  if(fragmentShaderModule == VK_NULL_HANDLE) {
    throw std::runtime_error(errorTitle + "failed to supply fragment shader!");
  }
  if(viewport.width <= 0 || viewport.height <= 0) {
    throw std::runtime_error(errorTitle + "failed to supply valid viewport!");
  }
  if(scissor.extent.width < 0 || scissor.extent.height < 0) {
    throw std::runtime_error(errorTitle + "supplied invalid scissor!");
  }
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setScissor(s32 offsetX, s32 offsetY, u32 width, u32 height)
{
  scissor.offset = { offsetX, offsetY};
  scissor.extent = { width, height };
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setVertexAttributes(VertexAtt vertexAtt, u32 bindingPoint)
{
  vertexInputBindingDesc.binding = bindingPoint;
  vertexInputBindingDesc.stride = vertexAtt.strideInBytes;
  vertexInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  if(vertexInputAttDescs != nullptr) { delete[] vertexInputAttDescs; }

  vertexInputAttDescs = new VkVertexInputAttributeDescription[vertexAtt.attributeCount];
  for(u32 i = 0; i < vertexAtt.attributeCount; ++i) {
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

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::setViewport(f32 originX, f32 originY, f32 originZ, u32 width, u32 height,
                                     f32 depth)
{
  viewport.x = originX;
  viewport.y = originY;
  viewport.minDepth = originZ;
  viewport.width = (f32)width;
  viewport.height = (f32)height;
  viewport.maxDepth = depth;
  return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::setDescriptorSetLayouts(VkDescriptorSetLayout* descriptorSetLayout, u32 count)
{
  pipelineLayoutCI.setLayoutCount = count; // descriptor set layouts
  pipelineLayoutCI.pSetLayouts = descriptorSetLayout; // num descriptor set layouts
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::
setFrontFace(VkFrontFace frontFace)
{
  frontFace = VK_FRONT_FACE_CLOCKWISE;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setCullMode(VkCullModeFlags cullModeFlags)
{
  cullModeFlags = VK_CULL_MODE_BACK_BIT;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setPolygonMode(VkPolygonMode polygonMode)
{
  polygonMode = VK_POLYGON_MODE_FILL;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setRenderPass(VkRenderPass renderPass)
{
  this->renderPass = renderPass;
  return *this;
}