#pragma once

#include "Platform.h"
#include <glm/glm.hpp>

#define min(x, y) (x < y ? x : y)
#define max(x, y) (x > y ? x : y)
#define clamp(lowerBound, upperBound, desiredVal) (desiredVal < lowerBound ? lowerBound : (desiredVal > upperBound ? upperBound : desiredVal))

void swap(float32* a, float32* b);
glm::mat4& reverseZ(glm::mat4& mat);
float32 getTime();
bool consume(bool& val);
void readFile(const char* filename, uint32* fileSize, char* fileBuffer);

class Consumabool {
public:
  bool value;
  Consumabool(bool startValue) {
    value = startValue;
  }
  void set() { value = true; }
  bool consume() {
    bool returnValue = value;
    value = false;
    return returnValue;
  }
};