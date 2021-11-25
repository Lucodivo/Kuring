#pragma once

#include "KuringTypes.h"
#include <glm/glm.hpp>

#define min(x, y) (x < y ? x : y)
#define max(x, y) (x > y ? x : y)
#define clamp(lowerBound, upperBound, desiredVal) (desiredVal < lowerBound ? lowerBound : (desiredVal > upperBound ? upperBound : desiredVal))

void swap(f32* a, f32* b);
glm::mat4& reverseZ(glm::mat4& mat);
f32 getTime();
bool consume(bool& val);
void readFile(const char* filename, u32* fileSize, char* fileBuffer);

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