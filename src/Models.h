/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Connor Haskins $
   ======================================================================== */

#pragma once

// PosColVertexAtt layout used in this example
struct PosColVertexAtt {
    real32 position[3];
    real32 color[3];
};

const PosColVertexAtt fullScreenQuadVertexAttData[] =
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
const uint32 fullScreenQuadVertexAttCount = ArrayCount(fullScreenQuadVertexAttData);
const uint32 fullScreenQuadVertexAttDataSize = ArrayCount(fullScreenQuadVertexAttData) * sizeof(PosColVertexAtt);
const uint32 fullScreenQuadIndexData[] = { 0, 1, 2,
                                           0, 3, 1};
const uint32 fullScreenQuadIndexCount = ArrayCount(fullScreenQuadIndexData);
const uint32 fullScreenQuadIndexDataSize = (fullScreenQuadIndexCount) * sizeof(uint32);


const PosColVertexAtt triangleVertexAttData[] =
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
const uint32 triangleVertexAttDataSize = ArrayCount(triangleVertexAttData) * sizeof(PosColVertexAtt);
const uint32 triangleIndexData[] = { 0, 1, 2};
const uint32 triangleIndexCount = ArrayCount(triangleIndexData);
const uint32 triangleIndexDataSize = (triangleIndexCount) * sizeof(uint32);
