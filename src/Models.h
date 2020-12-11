/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Connor Haskins $
   ======================================================================== */

#pragma once

struct VertexAttFormat {
  uint32 offsetInBytes;
  VkFormat format;
};

struct VertexAtt {
  uint32 strideInBytes;
  uint32 sizeInBytes;
  VertexAttFormat* attributeFormat;
  uint32 attributeCount;
  VkPrimitiveTopology primitiveTopology;
  void* data;
};

// PosColVertexAttDatum layout used in this example
struct PosColVertexAttDatum {
  float32 position[3];
  float32 color[3];
};
VertexAttFormat PosColVertexAttDatumFormat[]{
        { offsetof(PosColVertexAttDatum, position), VK_FORMAT_R32G32B32_SFLOAT },
        { offsetof(PosColVertexAttDatum, color), VK_FORMAT_R32G32B32_SFLOAT },
};
// TODO: test with file_access
const PosColVertexAttDatum quadPosColVertexAttData[] =
{
    { // VERTEX ATTRIBUTE
        { -1.0f,  -1.0f,  0.0f }, // POSITION
        {  1.0f,  0.0f,  0.0f }  // COLOR
    },
    {
        {  1.0f,  1.0f,  0.0f },
        {  1.0f,  1.0f,  1.0f }
    },
    {
        { -1.0f,  1.0f,  0.0f },
        {  0.0f,  0.0f,  1.0f }
    },
    {
        {  1.0f, -1.0f,  0.0f},
        {  0.0f,  1.0f,  0.0f}
    }
};
VertexAtt quadPosColVertexAtt {
        sizeof(PosColVertexAttDatum),
        ArrayCount(quadPosColVertexAttData) * sizeof(PosColVertexAttDatum),
        PosColVertexAttDatumFormat,
        ArrayCount(PosColVertexAttDatumFormat),
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        (void*)quadPosColVertexAttData
};

const float32 quadPosVertexAttData[][3] =
    {
        { -1.0f,  -1.0f,  0.0f }, // POSITION
        {  1.0f,  1.0f,  0.0f },
        { -1.0f,  1.0f,  0.0f },
        {  1.0f, -1.0f,  0.0f},
    };
const uint32 quadPosVertexAttCount = ArrayCount(quadPosVertexAttData);
const uint32 quadPosVertexAttDataSize = quadPosVertexAttCount * 3 * sizeof(float32);

const uint32 quadIndexData[] = {0, 1, 2,
                                0, 3, 1};
const uint32 quadIndexCount = ArrayCount(quadIndexData);
const uint32 quadIndexDataSize = quadIndexCount * sizeof(uint32);


const PosColVertexAttDatum triangleVertexAttData[] =
{
    { // VERTEX ATTRIBUTE
        {  0.0f, -0.5f,  0.0f }, // POSITION
        {  1.0f,  0.0f,  0.0f }  // COLOR
    },
    {
        {  0.5f,  0.5f,  0.0f },
        {  0.0f,  1.0f,  0.0f }
    },
    {
        { -0.5f,  0.5f,  0.0f },
        {  0.0f,  0.0f,  1.0f }
    }
};
const uint32 triangleVertexAttCount = ArrayCount(triangleVertexAttData);
const uint32 triangleVertexAttDataSize = ArrayCount(triangleVertexAttData) * sizeof(PosColVertexAttDatum);
const uint32 triangleIndexData[] = { 0, 1, 2};
const uint32 triangleIndexCount = ArrayCount(triangleIndexData);
const uint32 triangleIndexDataSize = (triangleIndexCount) * sizeof(uint32);
