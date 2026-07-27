#include "window.hpp"

#include <cstdio>

#include <GLFW/glfw3.h>

namespace Platform
{
    bool Window::Initialize(Graphics::RendererBackend backend, int width, int height, const char* title)
    {
        if (!glfwInit())
            return false;

        _glfwInitialized = true;
        glfwSetErrorCallback(ErrorCallback);

        if (backend == Graphics::RendererBackend::OPENGL)
        {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        }
        else
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }

        _handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!_handle)
        {
            Shutdown();
            return false;
        }

        _width = width;
        _height = height;
        _resizePending = false;

        glfwSetWindowUserPointer(_handle, this);
        glfwSetFramebufferSizeCallback(_handle, FramebufferSizeCallback);
        glfwSetKeyCallback(_handle, GlfwKeyCallback);
        return true;
    }

    void Window::Shutdown()
    {
        if (_handle)
            glfwDestroyWindow(_handle);
        _handle = nullptr;

        if (_glfwInitialized)
            glfwTerminate();
        _glfwInitialized = false;
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    bool Window::ShouldClose() const
    {
        return !_handle || glfwWindowShouldClose(_handle);
    }

    double Window::GetTime() const noexcept
    {
        return glfwGetTime();
    }

    void Window::SetKeyCallback(KeyCallback callback, void* context) noexcept
    {
        _keyCallback = callback;
        _keyContext = context;
    }

    GLFWwindow* Window::GetNativeHandle() const noexcept
    {
        return _handle;
    }

    int Window::GetWidth() const noexcept
    {
        return _width;
    }

    int Window::GetHeight() const noexcept
    {
        return _height;
    }

    bool Window::IsResizePending() const noexcept
    {
        return _resizePending;
    }

    void Window::ClearResizePending() noexcept
    {
        _resizePending = false;
    }

    void Window::ErrorCallback(int error, const char* description)
    {
        std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    }

    void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        auto* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!instance)
            return;

        instance->_width = width;
        instance->_height = height;
        instance->_resizePending = true;
    }

    void Window::GlfwKeyCallback(GLFWwindow* window, int key, int, int action, int)
    {
        auto* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        if (instance && instance->_keyCallback)
            instance->_keyCallback(instance->_keyContext, key, action);
    }
}
