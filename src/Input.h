#pragma once

#include "Platform.h"

#define GLFW_INCLUDE_NONE // ensure GLFW doesn't load OpenGL headers
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef void (*WindowSizeCallback)(void);

struct MouseCoord
{
  float64 x;
  float64 y;
};

struct ControllerAnalogStick
{
  int16 x;
  int16 y;
};

enum InputType
{
  KeyboardInput_Q, KeyboardInput_W, KeyboardInput_E, KeyboardInput_R,
  KeyboardInput_A, KeyboardInput_S, KeyboardInput_D, KeyboardInput_F,
  KeyboardInput_J, KeyboardInput_K, KeyboardInput_L, KeyboardInput_Semicolon,
  KeyboardInput_Shift_Left, KeyboardInput_Ctrl_Left, KeyboardInput_Alt_Left, KeyboardInput_Tab,
  KeyboardInput_Shift_Right, KeyboardInput_Ctrl_Right, KeyboardInput_Alt_Right, KeyboardInput_Enter,
  KeyboardInput_Esc, KeyboardInput_Backtick, KeyboardInput_1, KeyboardInput_2, KeyboardInput_3,
  KeyboardInput_Up, KeyboardInput_Down, KeyboardInput_Left, KeyboardInput_Right, KeyboardInput_Space,
  MouseInput_Left, MouseInput_Right, MouseInput_Middle, MouseInput_Back, MouseInput_Forward,
  MouseInput_Scroll, MouseInput_Movement,
  Controller1Input_A, Controller1Input_B, Controller1Input_X, Controller1Input_Y,
  Controller1Input_DPad_Up, Controller1Input_DPad_Down, Controller1Input_DPad_Left, Controller1Input_DPad_Right,
  Controller1Input_Shoulder_Left, Controller1Input_Trigger_Left, Controller1Input_Shoulder_Right, Controller1Input_Trigger_Right,
  Controller1Input_Start, Controller1Input_Select, Controller1Input_Analog_Left, Controller1Input_Analog_Right
};

enum InputState
{
  INPUT_HOT_PRESS = 1 << 0,
  INPUT_ACTIVE = 1 << 1,
  INPUT_HOT_RELEASE = 1 << 2,
  INPUT_INACTIVE = 1 << 3
};

void initializeInput(GLFWwindow* window);
void deinitializeInput();
void loadInputStateForFrame();

bool hotPress(InputType key); // returns true if input was just activated
bool hotRelease(InputType key); // returns true if input was just deactivated
bool isActive(InputType key); // returns true if key is pressed or held down
InputState getInputState(InputType key); // Note: for special use cases (ex: double click), use hotPress/hotRelease/isActive in most cases

MouseCoord getMousePosition();
MouseCoord getMouseDelta();
float32 getMouseScrollY();
int8 getControllerTriggerRaw_Left(); // NOTE: values range from 0 - 225 (255 minus trigger threshold)
int8 getControllerTriggerRaw_Right(); // NOTE: values range from 0 - 225 (255 minus trigger threshold)
float32 getControllerTrigger_Left(); // NOTE: values range from 0.0 - 1.0
float32 getControllerTrigger_Right(); // NOTE: values range from 0.0 - 1.0
Extent2D getWindowExtent();
ControllerAnalogStick getControllerAnalogStickLeft();
ControllerAnalogStick getControllerAnalogStickRight();

void closeWindow();
void enableCursor(bool enable);
bool isCursorEnabled();
bool isWindowFullscreen();
bool isWindowMinimized();
void toWindowedMode(uint32 width, uint32 height);
void toFullScreenMode();
void toggleWindowSize(uint32 width, uint32 height);

// NOTE: Call with NULL to unsubscribe
void subscribeWindowSizeCallback(WindowSizeCallback callback);