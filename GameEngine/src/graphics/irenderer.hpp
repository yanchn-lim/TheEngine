#pragma once

#include "drawcmd.hpp"
#include "camera.hpp"
#include <cstdint>

struct GLFWwindow;

namespace Graphics
{
	class IRenderer
	{
	public:
        virtual ~IRenderer() = default;

        virtual bool Init(GLFWwindow *) = 0;
        virtual void BeginFrame() = 0;
        virtual void Submit(const DrawCmd& cmd) = 0;
        virtual void EndFrame() = 0;
        virtual void SetCamera(const Camera2D& camera) = 0;
        virtual void OnResize(uint32_t width, uint32_t height) = 0;
        virtual void Shutdown() = 0;
	};
}