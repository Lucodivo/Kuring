/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Connor Haskins $
   ======================================================================== */

#pragma once

#include "Platform.h"
#include <vulkan/vulkan_core.h>

struct VertexAttFormat {
  uint32 offsetInBytes;
  VkFormat format;
};

struct VertexAttIndices {
  uint32 sizeInBytes;
  uint32 count;
  void* data;
};

struct VertexAtt {
  uint32 strideInBytes;
  uint32 sizeInBytes;
  VertexAttFormat* attributeFormat;
  uint32 attributeCount;
  VkPrimitiveTopology primitiveTopology;
  void* data;
  VertexAttIndices indices;
};

struct PosVertexAttDatum {
  float32 position[3];
};
extern VertexAttFormat posVertexAttDatumFormat[];

struct PosColVertexAttDatum {
  float32 position[3];
  float32 color[3];
};
extern VertexAttFormat posColVertexAttDatumFormat[];

extern VertexAtt quadPosColVertexAtt;
extern VertexAtt quadPosVertexAtt;