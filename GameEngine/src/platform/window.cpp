#include "window.hpp"

#include <cstdio>

#include <GLFW/glfw3.h>

#include "core/input.hpp"

namespace Ludus::Platform
{
    bool Window::Initialize(Ludus::Graphics::RendererBackend backend, int width, int height, const char* title)
    {
        if (!glfwInit())
            return false;

        _glfwInitialized = true;
        glfwSetErrorCallback(ErrorCallback);

        if (backend == Ludus::Graphics::RendererBackend::OPENGL)
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
		glfwSetMouseButtonCallback(_handle, GlfwMouseButtonCallback);
		glfwSetCursorPosCallback(_handle, GlfwCursorPositionCallback);
		glfwSetScrollCallback(_handle, GlfwScrollCallback);
		glfwSetWindowFocusCallback(_handle, GlfwFocusCallback);
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

	void Window::SetInput(Ludus::Input* input) noexcept
	{
		_input = input;
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

    void Window::GlfwKeyCallback(GLFWwindow* window, int key, int, int action, int modifiers)
    {
        auto* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
		if (instance && instance->_input)
		{
			const Ludus::InputAction inputAction = action == GLFW_RELEASE
				? Ludus::InputAction::Release
				: action == GLFW_REPEAT
					? Ludus::InputAction::Repeat
					: Ludus::InputAction::Press;
			instance->_input->ProcessKey(
				static_cast<Ludus::Key>(key),
				inputAction,
				static_cast<Ludus::Modifier>(modifiers));
		}
    }

	void Window::GlfwMouseButtonCallback(
		GLFWwindow* window, int button, int action, int modifiers)
	{
		auto* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
		if (!instance || !instance->_input)
			return;
		instance->_input->ProcessMouseButton(
			static_cast<Ludus::MouseButton>(button),
			action == GLFW_RELEASE
				? Ludus::InputAction::Release
				: Ludus::InputAction::Press,
			static_cast<Ludus::Modifier>(modifiers));
	}

	void Window::GlfwCursorPositionCallback(GLFWwindow* window, double x, double y)
	{
		auto* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
		if (instance && instance->_input)
			instance->_input->ProcessMouseMove({
				static_cast<float>(x), static_cast<float>(y) });
	}

	void Window::GlfwScrollCallback(GLFWwindow* window, double x, double y)
	{
		auto* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
		if (instance && instance->_input)
			instance->_input->ProcessScroll({
				static_cast<float>(x), static_cast<float>(y) });
	}

	void Window::GlfwFocusCallback(GLFWwindow* window, int focused)
	{
		auto* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
		if (instance && instance->_input)
			instance->_input->ProcessFocus(focused == GLFW_TRUE);
	}
}
