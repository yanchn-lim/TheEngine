#pragma once

#include "graphics/renderer_backend.hpp"

struct GLFWwindow;

namespace Platform
{
    class Window
    {
    public:
        using KeyCallback = void (*)(void* context, int key, int action);

        Window() = default;
        ~Window() = default;

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool Initialize(Graphics::RendererBackend backend, int width, int height, const char* title);
        void Shutdown();

        void PollEvents();
        bool ShouldClose() const;
        double GetTime() const noexcept;

        void SetKeyCallback(KeyCallback callback, void* context) noexcept;

        GLFWwindow* GetNativeHandle() const noexcept;
        int GetWidth() const noexcept;
        int GetHeight() const noexcept;
        bool IsResizePending() const noexcept;
        void ClearResizePending() noexcept;

    private:
        static void ErrorCallback(int error, const char* description);
        static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
        static void GlfwKeyCallback(GLFWwindow* window, int key, int scanCode, int action, int modifiers);

        GLFWwindow* _handle = nullptr;
        int _width = 0;
        int _height = 0;
        bool _resizePending = false;
        bool _glfwInitialized = false;
        KeyCallback _keyCallback = nullptr;
        void* _keyContext = nullptr;
    };
}
