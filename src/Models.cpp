#include "Models.h"

const uint32 quadIndexData[] = {0, 1, 2,
                                0, 3, 1};
const VertexAttIndices quadIndices {
        sizeof(quadIndexData),
        ArrayCount(quadIndexData),
        (void*)quadIndexData
};

VertexAttFormat posVertexAttDatumFormat[]{
        { (uint32)offsetof(PosVertexAttDatum, position), VK_FORMAT_R32G32B32_SFLOAT }
};

VertexAttFormat posColVertexAttDatumFormat[]{
        { (uint32)offsetof(PosColVertexAttDatum, position), VK_FORMAT_R32G32B32_SFLOAT },
        { (uint32)offsetof(PosColVertexAttDatum, color), VK_FORMAT_R32G32B32_SFLOAT },
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
const uint32 triangleVertexAttCount = ArrayCount(triangleVertexAttData);
const uint32 triangleVertexAttDataSize = ArrayCount(triangleVertexAttData) * sizeof(PosColVertexAttDatum);
const uint32 triangleIndexData[] = { 0, 1, 2};
const uint32 triangleIndexCount = ArrayCount(triangleIndexData);
const uint32 triangleIndexDataSize = (triangleIndexCount) * sizeof(uint32);