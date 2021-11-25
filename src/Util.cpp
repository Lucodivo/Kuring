#include "Util.h"

#include <time.h>
#include <fstream>

void swap(f32* a, f32* b)
{
  f32 tmp = *a;
  *a = *b;
  *b = tmp;
}

glm::mat4& reverseZ(glm::mat4& mat)
{
  mat[0][2] = -mat[0][2];
  mat[1][2] = -mat[1][2];
  mat[2][2] = -mat[2][2];
  mat[3][2] = -mat[3][2];
  return mat;
}

f32 getTime() {
  clock_t time = clock();
  return (f32)time / CLOCKS_PER_SEC;
}

void readFile(const char* filename, u32* fileSize, char* fileBuffer) {
  // Read at end to get file size
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error(std::string("failed to open file:") + filename);
  }

  *fileSize = (u32) file.tellg();

  if(fileBuffer != nullptr) {
    file.seekg(0);
    file.read(fileBuffer, *fileSize);
  }

  file.close();
}