#include "input.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>

namespace Engine
{
    namespace
    {
        InputState  s_State{};
        EventBus* s_EventBus = nullptr;
        GLFWwindow* s_Window = nullptr;

        // GLFW Callbacks ------------------------------------------------------

        void KeyCallback(GLFWwindow* /*window*/, int key, int /*scancode*/,
            int action, int /*mods*/)
        {
            if (key < 0 || key >= MaxKeys)
                return;

            switch (action)
            {
            case GLFW_PRESS:
            {
                if (!s_State.keyDown[key])
                {
                    s_State.keyPressed[key] = true;
                }
                s_State.keyDown[key] = true;

                if (s_EventBus)
                {
                    s_EventBus->Emit(KeyPressedEvent{
                        static_cast<KeyCode>(key),
                        false // not a repeat
                        });
                }
            } break;

            case GLFW_RELEASE:
            {
                s_State.keyDown[key] = false;
                s_State.keyReleased[key] = true;

                if (s_EventBus)
                {
                    s_EventBus->Emit(KeyReleasedEvent{
                        static_cast<KeyCode>(key)
                        });
                }
            } break;

            case GLFW_REPEAT:
            {
                if (s_EventBus)
                {
                    s_EventBus->Emit(KeyPressedEvent{
                        static_cast<KeyCode>(key),
                        true // repeated
                        });
                }
            } break;
            }
        }

        void MouseButtonCallback(GLFWwindow* window, int button,
            int action, int /*mods*/)
        {
            if (button < 0 || button >= MaxMouseButtons)
                return;

            double x = 0.0, y = 0.0;
            glfwGetCursorPos(window, &x, &y);

            switch (action)
            {
            case GLFW_PRESS:
            {
                if (!s_State.mouseDown[button])
                {
                    s_State.mousePressed[button] = true;
                }
                s_State.mouseDown[button] = true;

                if (s_EventBus)
                {
                    s_EventBus->Emit(MouseButtonPressedEvent{
                        static_cast<MouseButton>(button),
                        x,
                        y
                        });
                }
            } break;

            case GLFW_RELEASE:
            {
                s_State.mouseDown[button] = false;
                s_State.mouseReleased[button] = true;

                if (s_EventBus)
                {
                    s_EventBus->Emit(MouseButtonReleasedEvent{
                        static_cast<MouseButton>(button),
                        x,
                        y
                        });
                }
            } break;
            }
        }

        void CursorPosCallback(GLFWwindow* /*window*/, double x, double y)
        {
            double dx = x - s_State.mouseX;
            double dy = y - s_State.mouseY;

            s_State.mouseX = x;
            s_State.mouseY = y;
            s_State.mouseDeltaX += dx;
            s_State.mouseDeltaY += dy;

            if (s_EventBus)
            {
                s_EventBus->Emit(MouseMovedEvent{
                    x, y, dx, dy
                    });
            }
        }

        void ScrollCallback(GLFWwindow* /*window*/, double offsetX, double offsetY)
        {
            s_State.scrollX += offsetX;
            s_State.scrollY += offsetY;

            if (s_EventBus)
            {
                s_EventBus->Emit(MouseScrolledEvent{
                    offsetX,
                    offsetY
                    });
            }
        }
    } // anonymous namespace

    // ---- InputSystem Implementation -----------------------------------------

    namespace InputSystem
    {
        void Initialize(GLFWwindow* window, EventBus* eventBus)
        {
            s_Window = window;
            s_EventBus = eventBus;

            // Clear state
            s_State = InputState{};

            // Set GLFW callbacks
            glfwSetKeyCallback(window, KeyCallback);
            glfwSetMouseButtonCallback(window, MouseButtonCallback);
            glfwSetCursorPosCallback(window, CursorPosCallback);
            glfwSetScrollCallback(window, ScrollCallback);
        }

        void BeginFrame()
        {
            // Reset per-frame flags and deltas
            for (int i = 0; i < MaxKeys; ++i)
            {
                s_State.keyPressed[i] = false;
                s_State.keyReleased[i] = false;
            }

            for (int i = 0; i < MaxMouseButtons; ++i)
            {
                s_State.mousePressed[i] = false;
                s_State.mouseReleased[i] = false;
            }

            s_State.mouseDeltaX = 0.0;
            s_State.mouseDeltaY = 0.0;
            s_State.scrollX = 0.0;
            s_State.scrollY = 0.0;
        }

        const InputState& GetState()
        {
            return s_State;
        }

        bool IsKeyDown(KeyCode key)
        {
            if (key < 0 || key >= MaxKeys)
                return false;
            return s_State.keyDown[key];
        }

        bool WasKeyPressed(KeyCode key)
        {
            if (key < 0 || key >= MaxKeys)
                return false;
            return s_State.keyPressed[key];
        }

        bool WasKeyReleased(KeyCode key)
        {
            if (key < 0 || key >= MaxKeys)
                return false;
            return s_State.keyReleased[key];
        }

        bool IsMouseButtonDown(MouseButton button)
        {
            if (button < 0 || button >= MaxMouseButtons)
                return false;
            return s_State.mouseDown[button];
        }

        bool WasMouseButtonPressed(MouseButton button)
        {
            if (button < 0 || button >= MaxMouseButtons)
                return false;
            return s_State.mousePressed[button];
        }

        bool WasMouseButtonReleased(MouseButton button)
        {
            if (button < 0 || button >= MaxMouseButtons)
                return false;
            return s_State.mouseReleased[button];
        }

        double GetMouseX() { return s_State.mouseX; }
        double GetMouseY() { return s_State.mouseY; }
        double GetMouseDeltaX() { return s_State.mouseDeltaX; }
        double GetMouseDeltaY() { return s_State.mouseDeltaY; }
        double GetScrollX() { return s_State.scrollX; }
        double GetScrollY() { return s_State.scrollY; }
    }
}
