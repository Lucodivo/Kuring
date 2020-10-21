#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 fragColor;

#define MAX_STEPS 60
#define HIT_DIST 0.01
#define MISS_DIST 200.0
#define MISS 120012

const vec3 rayOrigin = vec3(0.0, 0.0, 0.0);
const vec3 sphereCenter = vec3(0.0, 0.0, -8.0);
const float sphereRadius = 3.0;
const vec3 sphereColor = vec3(1.0, 0.15, 0.5);
const float planeHeight = -3.0;
const vec3 planeColor = vec3(0.3, 0.5, 1.0);
const vec3 missColor = vec3(0.0, 0.0, 0.0);
const vec2 viewPortResolution = vec2(1200, 1200);

float sdXZPlane(vec3 rayPosition, float planeHeight) {
  return abs(rayPosition.y + planeHeight);
}

float sdSphere(vec3 rayPosition) {
  return length(rayPosition) - 1.0;
}

// vec4 return value split into:
// - vec3 color of object hit
// - float number of iterations
vec4 sdScene(vec3 rayOrigin, vec3 rayDir) {
   float distanceTraveled = 0;
   float sphereDist;
   float planeDist;
   for(int iterations = 0; iterations < MAX_STEPS; ++iterations) {
     sphereDist = sdSphere((rayOrigin - sphereCenter) / sphereRadius) * sphereRadius;
     planeDist = sdXZPlane(rayOrigin, planeHeight);
     float distance = min(sphereDist, planeDist);
     if(distance < HIT_DIST) {
       vec3 color = sphereDist < planeDist ? sphereColor : planeColor;
       return vec4(color, float(iterations));
     }
     rayOrigin += distance * rayDir;
     distanceTraveled += distance;
     if(distanceTraveled > MISS_DIST) {
       return vec4(missColor, MISS);
     }
   }
   return vec4(missColor, MISS);
}

void main() {
  // Move (0,0) from bottom left to center
  // Coordinate system goes from [-viewPortResolution / 2, viewPortResolution / 2]
  vec2 pixelCoord = gl_FragCoord.xy-0.5*viewPortResolution.xy;
  float pixelWidth = 1.0 / viewPortResolution.y;
  // Scale y value to [-0.5, 0.5], scale x by same factor
  pixelCoord = pixelCoord * pixelWidth;
  vec3 rayDir = normalize(vec3(pixelCoord, -1.0));

  vec4 hitResult = sdScene(rayOrigin, rayDir);
  if(hitResult.a == MISS) discard;
  outColor = vec4(hitResult.rgb * (1.0 - (hitResult.a / MAX_STEPS)), 1.0);
}