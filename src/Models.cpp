#include "Models.h"

const u32 quadIndexData[] = {0, 1, 2,
                             0, 3, 1};
const VertexAttIndices quadIndices {
        sizeof(quadIndexData),
        ArrayCount(quadIndexData),
        (void*)quadIndexData
};

VertexAttFormat posVertexAttDatumFormat[]{
        { (u32)offsetof(PosVertexAttDatum, position), VK_FORMAT_R32G32B32_SFLOAT }
};

VertexAttFormat posColVertexAttDatumFormat[]{
        { (u32)offsetof(PosColVertexAttDatum, position), VK_FORMAT_R32G32B32_SFLOAT },
        { (u32)offsetof(PosColVertexAttDatum, color),    VK_FORMAT_R32G32B32_SFLOAT },
};

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
VertexAtt quadPosColVertexAtt{
        sizeof(PosColVertexAttDatum),
        ArrayCount(quadPosColVertexAttData) * sizeof(PosColVertexAttDatum),
        posColVertexAttDatumFormat,
        ArrayCount(posColVertexAttDatumFormat),
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        (void*)quadPosColVertexAttData,
        quadIndices
};

const PosVertexAttDatum quadPosVertexAttData[] =
        {
                { -1.0f,  -1.0f,  0.0f }, // POSITION
                {  1.0f,  1.0f,  0.0f },
                { -1.0f,  1.0f,  0.0f },
                {  1.0f, -1.0f,  0.0f},
        };
VertexAtt quadPosVertexAtt {
        sizeof(PosVertexAttDatum),
        ArrayCount(quadPosVertexAttData) * sizeof(PosVertexAttDatum),
        posVertexAttDatumFormat,
        ArrayCount(posVertexAttDatumFormat),
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        (void*)quadPosVertexAttData,
        quadIndices
};


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
const u32 triangleVertexAttCount = ArrayCount(triangleVertexAttData);
const u32 triangleVertexAttDataSize = ArrayCount(triangleVertexAttData) * sizeof(PosColVertexAttDatum);
const u32 triangleIndexData[] = {0, 1, 2};
const u32 triangleIndexCount = ArrayCount(triangleIndexData);
const u32 triangleIndexDataSize = (triangleIndexCount) * sizeof(u32);