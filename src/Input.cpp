#include <Windows.h>

#include "Input.h"

#include "Util.h"
#include <Xinput.h>
#include <iostream>
#include <vector>
#include <unordered_map>

file_access void setKeyState(uint32 glfwKey, InputType keyboardInput);
file_access void setMouseState(uint32 glfwKey, InputType mouseInput);
file_access void setControllerState(int16 gamepadFlags, uint32 xInputButtonFlag, InputType controllerInput);
file_access void loadXInput();

file_access void glfw_mouse_scroll_callback(GLFWwindow* wndw, float64 xOffset, float64 yOffset);
file_access void glfw_framebuffer_size_callback(GLFWwindow* wndw, int32 width, int32 height);

file_access Consumabool windowModeChangeTossNextInput = Consumabool(false);
file_access Extent2D globalWindowExtent = Extent2D{ 0, 0 };
file_access MouseCoord globalMouseScroll = MouseCoord{ 0.0f, 0.0f };
file_access MouseCoord mousePosition = { 0.0f, 0.0f };
file_access MouseCoord mouseDelta = { 0.0f, 0.0f };
file_access ControllerAnalogStick analogStickLeft = { 0, 0 };
file_access ControllerAnalogStick analogStickRight = { 0, 0 };
file_access float32 mouseScrollY = 0.0f;
file_access int8 controller1TriggerLeftValue = 0;
file_access int8 controller1TriggerRightValue = 0;
file_access std::unordered_map<InputType, InputState>* inputState = nullptr;
file_access WindowSizeCallback windowSizeCallback = nullptr;
file_access GLFWwindow* window = nullptr;

// NOTE: Casey Muratori's efficient way of handling function pointers, Handmade Hero episode 6 @ 22:06 & 1:00:21
// NOTE: Allows us to quickly change the function parameters & return type in one place and cascade throughout the rest
// NOTE: of the code if need be.
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState) // succinct way to define function of this type in future
typedef X_INPUT_GET_STATE(x_input_get_state); // succinct way to define function pointer of type above in the future
X_INPUT_GET_STATE(XInputGetStateStub) // create stub function of type above
{
  return (ERROR_DEVICE_NOT_CONNECTED);
}
file_access x_input_get_state* XInputGetState_ = XInputGetStateStub; // Create a function pointer of type above to point to stub
#define XInputGetState XInputGetState_ // Allow us to use XInputGetState method name without conflicting with definition in Xinput.h

void initializeInput(GLFWwindow* wndw)
{
  window = wndw;
  glfwSetScrollCallback(window, glfw_mouse_scroll_callback);
  glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);

  inputState = new std::unordered_map<InputType, InputState>();

  int32 framebufferWidth, framebufferHeight;
  glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
  framebufferWidth = max(0, framebufferWidth);
  framebufferHeight = max(0, framebufferHeight);
  globalWindowExtent = Extent2D{ (uint32)framebufferWidth, (uint32)framebufferHeight};

  loadXInput();
}

void deinitializeInput()
{
  glfwSetScrollCallback(window, nullptr);
  glfwSetFramebufferSizeCallback(window, nullptr);
  window = nullptr;
  windowSizeCallback = nullptr;

  delete inputState;

  windowModeChangeTossNextInput.consume();
  globalWindowExtent = Extent2D{ 0, 0 };
  globalMouseScroll = MouseCoord{ 0.0f, 0.0f };
  mousePosition = { 0.0f, 0.0f };
  mouseDelta = { 0.0f, 0.0f };
  analogStickLeft = { 0, 0 };
  analogStickRight = { 0, 0 };
  mouseScrollY = 0.0f;
}

InputState getInputState(InputType key) {
  std::unordered_map<InputType, InputState>::iterator inputSearch = inputState->find(key);
  return inputSearch != inputState->end() ? inputSearch->second : INPUT_INACTIVE;
}

bool hotPress(InputType key) {
  return getInputState(key) & INPUT_HOT_PRESS;
}

bool hotRelease(InputType key) {
  return getInputState(key) & INPUT_HOT_RELEASE;
}

bool isActive(InputType key) {
  return getInputState(key) & (INPUT_HOT_PRESS | INPUT_ACTIVE);
}

MouseCoord getMousePosition() {
  return mousePosition;
}

MouseCoord getMouseDelta() {
  return mouseDelta;
}

float32 getMouseScrollY() {
  return mouseScrollY;
}

Extent2D getWindowExtent() {
  return globalWindowExtent;
}

ControllerAnalogStick getControllerAnalogStickLeft(){
  return analogStickLeft;
}
ControllerAnalogStick getControllerAnalogStickRight() {
  return analogStickRight;
}

void setKeyState(uint32 glfwKey, InputType keyboardInput)
{
  std::unordered_map<InputType, InputState>::iterator keyIterator = inputState->find(keyboardInput);
  InputState oldKeyState = keyIterator != inputState->end() ? keyIterator->second : INPUT_INACTIVE;
  if (glfwGetKey(window, glfwKey) == GLFW_PRESS)
  {
    if(oldKeyState & INPUT_HOT_PRESS) {
      (*inputState)[keyboardInput] = INPUT_ACTIVE;
    } else if(oldKeyState ^ INPUT_ACTIVE) {
      (*inputState)[keyboardInput] = INPUT_HOT_PRESS;
    }
  } else if(oldKeyState & (INPUT_HOT_PRESS | INPUT_ACTIVE)) {
    (*inputState)[keyboardInput] = INPUT_HOT_RELEASE;
  } else if(oldKeyState & INPUT_HOT_RELEASE) { // only erase if there is something to be erased
    inputState->erase(keyIterator);
  }
}

void setMouseState(uint32 glfwKey, InputType mouseInput)
{
  std::unordered_map<InputType, InputState>::iterator mouseInputIterator = inputState->find(mouseInput);
  InputState oldMouseInputState = mouseInputIterator != inputState->end() ? mouseInputIterator->second : INPUT_INACTIVE;
  if (glfwGetMouseButton(window, glfwKey) == GLFW_PRESS)
  {
    if(oldMouseInputState & INPUT_HOT_PRESS) {
      (*inputState)[mouseInput] = INPUT_ACTIVE;
    } else if(oldMouseInputState ^ INPUT_ACTIVE) {
      (*inputState)[mouseInput] = INPUT_HOT_PRESS;
    }
  } else if(oldMouseInputState & (INPUT_HOT_PRESS | INPUT_ACTIVE)) {
    (*inputState)[mouseInput] = INPUT_HOT_RELEASE;
  } else if(oldMouseInputState & INPUT_HOT_RELEASE) { // only erase if there is something to be erased
    inputState->erase(mouseInputIterator);
  }
}

void setControllerState(int16 gamepadFlags, uint32 xInputButtonFlag, InputType controllerInput)
{
  std::unordered_map<InputType, InputState>::iterator controllerInputIterator = inputState->find(controllerInput);
  InputState oldControllerInputState = controllerInputIterator != inputState->end() ? controllerInputIterator->second : INPUT_INACTIVE;
  if (gamepadFlags & xInputButtonFlag)
  {
    if (oldControllerInputState & INPUT_HOT_PRESS)
    {
      (*inputState)[controllerInput] = INPUT_ACTIVE;
    } else if (oldControllerInputState ^ INPUT_ACTIVE)
    {
      (*inputState)[controllerInput] = INPUT_HOT_PRESS;
    }
  } else if (oldControllerInputState & (INPUT_HOT_PRESS | INPUT_ACTIVE))
  {
    (*inputState)[controllerInput] = INPUT_HOT_RELEASE;
  } else if (oldControllerInputState & INPUT_HOT_RELEASE)
  { // only erase if there is something to be erased
    inputState->erase(controllerInputIterator);
  }
}

void loadInputStateForFrame() {
  // keyboard state
  {
    setKeyState(GLFW_KEY_Q, KeyboardInput_Q);
    setKeyState(GLFW_KEY_W, KeyboardInput_W);
    setKeyState(GLFW_KEY_E, KeyboardInput_E);
    setKeyState(GLFW_KEY_R, KeyboardInput_R);
    setKeyState(GLFW_KEY_A, KeyboardInput_A);
    setKeyState(GLFW_KEY_S, KeyboardInput_S);
    setKeyState(GLFW_KEY_D, KeyboardInput_D);
    setKeyState(GLFW_KEY_F, KeyboardInput_F);
    setKeyState(GLFW_KEY_J, KeyboardInput_J);
    setKeyState(GLFW_KEY_K, KeyboardInput_K);
    setKeyState(GLFW_KEY_L, KeyboardInput_L);
    setKeyState(GLFW_KEY_SEMICOLON, KeyboardInput_Semicolon);
    setKeyState(GLFW_KEY_LEFT_SHIFT, KeyboardInput_Shift_Left);
    setKeyState(GLFW_KEY_LEFT_CONTROL, KeyboardInput_Ctrl_Left);
    setKeyState(GLFW_KEY_LEFT_ALT, KeyboardInput_Alt_Left);
    setKeyState(GLFW_KEY_TAB, KeyboardInput_Tab);
    setKeyState(GLFW_KEY_RIGHT_SHIFT, KeyboardInput_Shift_Right);
    setKeyState(GLFW_KEY_RIGHT_CONTROL, KeyboardInput_Ctrl_Right);
    setKeyState(GLFW_KEY_RIGHT_ALT, KeyboardInput_Alt_Right);
    setKeyState(GLFW_KEY_ENTER, KeyboardInput_Enter);
    setKeyState(GLFW_KEY_ESCAPE, KeyboardInput_Esc);
    setKeyState(GLFW_KEY_GRAVE_ACCENT, KeyboardInput_Backtick);
    setKeyState(GLFW_KEY_1, KeyboardInput_1);
    setKeyState(GLFW_KEY_2, KeyboardInput_2);
    setKeyState(GLFW_KEY_3, KeyboardInput_3);
    setKeyState(GLFW_KEY_UP, KeyboardInput_Up);
    setKeyState(GLFW_KEY_DOWN, KeyboardInput_Down);
    setKeyState(GLFW_KEY_LEFT, KeyboardInput_Left);
    setKeyState(GLFW_KEY_RIGHT, KeyboardInput_Right);
    setKeyState(GLFW_KEY_SPACE, KeyboardInput_Space);
  }

  // mouse state
  {
    setMouseState(GLFW_MOUSE_BUTTON_LEFT, MouseInput_Left);
    setMouseState(GLFW_MOUSE_BUTTON_RIGHT, MouseInput_Right);
    setMouseState(GLFW_MOUSE_BUTTON_MIDDLE, MouseInput_Middle);
    setMouseState(GLFW_MOUSE_BUTTON_4, MouseInput_Back);
    setMouseState(GLFW_MOUSE_BUTTON_5, MouseInput_Forward);

    // mouse movement state management
    {
      MouseCoord newMouseCoord;

      glfwGetCursorPos(window, &newMouseCoord.x, &newMouseCoord.y);

      // NOTE: We do not consume mouse input on window size changes as it results in unwanted values
      mouseDelta = windowModeChangeTossNextInput.consume() ? MouseCoord{0.0f, 0.0f} : MouseCoord{newMouseCoord.x - mousePosition.x, newMouseCoord.y - mousePosition.y};
      mousePosition = newMouseCoord;

      std::unordered_map<InputType, InputState>::iterator movementIterator = inputState->find(MouseInput_Movement);
      bool movementWasActive = movementIterator != inputState->end();
      if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f)
      {
        if(!movementWasActive) {
          (*inputState)[MouseInput_Movement] = INPUT_ACTIVE;
        }
      } else if (movementWasActive) // scroll no longer active
      {
        inputState->erase(movementIterator);
      }
    }

    // mouse scroll state management
    {
      std::unordered_map<InputType, InputState>::iterator scrollIterator = inputState->find(MouseInput_Scroll);
      bool scrollWasActive = scrollIterator != inputState->end();
      mouseScrollY = (float32)globalMouseScroll.y;
      globalMouseScroll.y = 0.0f; // NOTE: Set to 0.0f to signify that the result has been consumed
      if (mouseScrollY != 0.0f && !scrollWasActive)
      {
        (*inputState)[MouseInput_Scroll] = INPUT_ACTIVE;
      } else if (scrollWasActive) // scroll no longer active
      {
        inputState->erase(scrollIterator);
      }
    }
  }

  // TODO: Add support for multiple controllers?
  const uint32 controllerIndex = 0;
  XINPUT_STATE controllerState;
  if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
  {
    // the controller is plugged in
    int16 gamepadButtonFlags = controllerState.Gamepad.wButtons;
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_A, Controller1Input_A);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_B, Controller1Input_B);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_X, Controller1Input_X);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_Y, Controller1Input_Y);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_DPAD_UP, Controller1Input_DPad_Up);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_DPAD_DOWN, Controller1Input_DPad_Down);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_DPAD_LEFT, Controller1Input_DPad_Left);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_DPAD_RIGHT, Controller1Input_DPad_Right);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_LEFT_SHOULDER, Controller1Input_Shoulder_Left);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_RIGHT_SHOULDER, Controller1Input_Shoulder_Right);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_START, Controller1Input_Start);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_BACK, Controller1Input_Select);

    bool analogStickLeftWasActive = analogStickLeft.x != 0 || analogStickLeft.y != 0;
    analogStickLeft = { controllerState.Gamepad.sThumbLX, controllerState.Gamepad.sThumbLY };
    if(analogStickLeft.x > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && analogStickLeft.x < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
      analogStickLeft.x = 0; // deadzone
    }
    if(analogStickLeft.y > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && analogStickLeft.y < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
      analogStickLeft.y = 0; // deadzone
    }
    bool analogStickLeftIsActive = analogStickLeft.x != 0 || analogStickLeft.y != 0;
    if (!analogStickLeftWasActive && analogStickLeftIsActive)
    {
      (*inputState)[Controller1Input_Analog_Left] = INPUT_ACTIVE;
    } else if (analogStickLeftWasActive && !analogStickLeftIsActive)
    {
      inputState->erase(Controller1Input_Analog_Left);
    }

    bool analogStickRightWasActive = analogStickRight.x != 0 || analogStickRight.y != 0;
    analogStickRight = { controllerState.Gamepad.sThumbRX, controllerState.Gamepad.sThumbRY };
    if(analogStickRight.x > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && analogStickRight.x < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
    {
      analogStickRight.x = 0; // deadzone
    }
    if(analogStickRight.y > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && analogStickRight.y < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
    {
      analogStickRight.y = 0; // deadzone
    }
    bool analogStickRightIsActive = analogStickRight.x != 0 || analogStickRight.y != 0;
    if (!analogStickRightWasActive && analogStickRightIsActive)
    {
      (*inputState)[Controller1Input_Analog_Right] = INPUT_ACTIVE;
    } else if (analogStickRightWasActive && !analogStickRightIsActive)
    {
      inputState->erase(Controller1Input_Analog_Right);
    }

    std::unordered_map<InputType, InputState>::iterator triggerLeftIterator = inputState->find(Controller1Input_Trigger_Left);
    InputState oldTriggerLeftState = triggerLeftIterator != inputState->end() ? triggerLeftIterator->second : INPUT_INACTIVE;
    if (controllerState.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
    {
      if(oldTriggerLeftState & INPUT_HOT_PRESS) {
        (*inputState)[Controller1Input_Trigger_Left] = INPUT_ACTIVE;
      } else if(oldTriggerLeftState ^ INPUT_ACTIVE) {
        (*inputState)[Controller1Input_Trigger_Left] = INPUT_HOT_PRESS;
      }
      controller1TriggerLeftValue = controllerState.Gamepad.bLeftTrigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    } else if(oldTriggerLeftState & (INPUT_HOT_PRESS | INPUT_ACTIVE)) {
      (*inputState)[Controller1Input_Trigger_Left] = INPUT_HOT_RELEASE;
      controller1TriggerLeftValue = 0;
    } else if(oldTriggerLeftState & INPUT_HOT_RELEASE) { // only erase if there is something to be erased
      inputState->erase(triggerLeftIterator);
    }

    std::unordered_map<InputType, InputState>::iterator triggerRightIterator = inputState->find(Controller1Input_Trigger_Right);
    InputState oldTriggerRightState = triggerRightIterator != inputState->end() ? triggerRightIterator->second : INPUT_INACTIVE;
    if (controllerState.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
    {
      if(oldTriggerRightState & INPUT_HOT_PRESS) {
        (*inputState)[Controller1Input_Trigger_Right] = INPUT_ACTIVE;
      } else if(oldTriggerRightState ^ INPUT_ACTIVE) {
        (*inputState)[Controller1Input_Trigger_Right] = INPUT_HOT_PRESS;
      }
      controller1TriggerRightValue = controllerState.Gamepad.bRightTrigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    } else if(oldTriggerRightState & (INPUT_HOT_PRESS | INPUT_ACTIVE)) {
      (*inputState)[Controller1Input_Trigger_Right] = INPUT_HOT_RELEASE;
      controller1TriggerRightValue = 0;
    } else if(oldTriggerRightState & INPUT_HOT_RELEASE) { // only erase if there is something to be erased
      inputState->erase(triggerRightIterator);
    }
  }
}

// NOTE: values range from 0 to 225 (255 minus trigger threshold)
int8 getControllerTriggerRaw_Right() {
  return controller1TriggerRightValue;
}

// NOTE: values range from 0 to 225 (255 minus trigger threshold)
int8 getControllerTriggerRaw_Left() {
  return controller1TriggerLeftValue;
}

// NOTE: values range from 0.0 - 1.0
float32 getControllerTrigger_Right() {
  return (float32)controller1TriggerRightValue / (255 - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

// NOTE: values range from 0.0 - 1.0
float32 getControllerTrigger_Left() {
  return (float32)controller1TriggerLeftValue / (255 - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

// Callback function for when user scrolls with mouse wheel
void glfw_mouse_scroll_callback(GLFWwindow* w, float64 xOffset, float64 yOffset)
{
  globalMouseScroll.y = yOffset;
}

void subscribeWindowSizeCallback(WindowSizeCallback callback)
{
  windowSizeCallback = callback;
}

// NOTE: returns (0,0) when no longer on screen
void glfw_framebuffer_size_callback(GLFWwindow* w, int32 width, int32 height)
{
  local_access bool globalWindowMinimized = false;
  // NOTE: Currently just ignoring minimize.
  if(width <= 0 || height <= 0) {
    globalWindowMinimized = true;
    return;
  } else if(globalWindowMinimized) {
    globalWindowMinimized = false;
    // return if no change in size has been made to the framebuffer since minimization
    if(globalWindowExtent.width == (uint32)width && globalWindowExtent.height == (uint32)height)
    {
      return;
    }
  }

  windowModeChangeTossNextInput.set();
  globalWindowExtent = { (uint32)width, (uint32)height };

  if(windowSizeCallback != nullptr) {
    windowSizeCallback();
  }
}

void closeWindow() {
  glfwSetWindowShouldClose(window, true);
}

void enableCursor(bool enable)
{
  glfwSetInputMode(window, GLFW_CURSOR, enable ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
  windowModeChangeTossNextInput.set();
}

bool isCursorEnabled()
{
  return glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL;
}

file_access void loadXInput()
{
  HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
  if (!XInputLibrary)
  {
    XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
  }
  if (!XInputLibrary)
  {
    XInputLibrary = LoadLibraryA("xinput1_3.dll");
  }
  if (XInputLibrary)
  {
    XInputGetState = (x_input_get_state*) GetProcAddress(XInputLibrary, "XInputGetState");
    if (!XInputGetState)
    {
      XInputGetState = XInputGetStateStub;
    }
  } else
  {
    std::cout << "Failed to load XInput" << std::endl;
    exit(-1);
  }
}