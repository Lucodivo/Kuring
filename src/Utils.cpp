/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Connor Haskins $
   ======================================================================== */
#pragma once

#include <fstream>
#include "Platform.h"

static void readFile(const char* filename, uint32* fileSize, char* fileBuffer) {
    // Read at end to get file size
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    
    *fileSize = (uint32) file.tellg();

    if(fileBuffer != nullptr) {
        file.seekg(0);
        file.read(fileBuffer, *fileSize);
    }

    file.close();
}
