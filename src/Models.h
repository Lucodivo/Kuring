/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Connor Haskins $
   ======================================================================== */

#pragma once

#include "KuringTypes.h"
#include <vulkan/vulkan_core.h>

struct VertexAttFormat {
  u32 offsetInBytes;
  VkFormat format;
};

struct VertexAttIndices {
  u32 sizeInBytes;
  u32 count;
  void* data;
};

struct VertexAtt {
  u32 strideInBytes;
  u32 sizeInBytes;
  VertexAttFormat* attributeFormat;
  u32 attributeCount;
  VkPrimitiveTopology primitiveTopology;
  void* data;
  VertexAttIndices indices;
};

struct PosVertexAttDatum {
  f32 position[3];
};
extern VertexAttFormat posVertexAttDatumFormat[];

struct PosColVertexAttDatum {
  f32 position[3];
  f32 color[3];
};
extern VertexAttFormat posColVertexAttDatumFormat[];

extern VertexAtt quadPosColVertexAtt;
extern VertexAtt quadPosVertexAtt;