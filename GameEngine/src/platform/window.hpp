#pragma once

#include "graphics/renderer_backend.hpp"

struct GLFWwindow;

namespace Ludus
{
	class Input;
}

namespace Ludus::Platform
{
    class Window
    {
    public:
        Window() = default;
        ~Window() = default;

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool Initialize(Ludus::Graphics::RendererBackend backend, int width, int height, const char* title);
        void Shutdown();

        void PollEvents();
        bool ShouldClose() const;
        double GetTime() const noexcept;

		void SetInput(Ludus::Input* input) noexcept;

        GLFWwindow* GetNativeHandle() const noexcept;
        int GetWidth() const noexcept;
        int GetHeight() const noexcept;
        bool IsResizePending() const noexcept;
        void ClearResizePending() noexcept;

    private:
        static void ErrorCallback(int error, const char* description);
        static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
        static void GlfwKeyCallback(GLFWwindow* window, int key, int scanCode, int action, int modifiers);
		static void GlfwMouseButtonCallback(GLFWwindow* window, int button, int action, int modifiers);
		static void GlfwCursorPositionCallback(GLFWwindow* window, double x, double y);
		static void GlfwScrollCallback(GLFWwindow* window, double x, double y);
		static void GlfwFocusCallback(GLFWwindow* window, int focused);

        GLFWwindow* _handle = nullptr;
        int _width = 0;
        int _height = 0;
        bool _resizePending = false;
        bool _glfwInitialized = false;
		Ludus::Input* _input = nullptr;
    };
}
