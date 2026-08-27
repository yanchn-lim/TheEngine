#pragma once

#include "graphics/imgui_backend.hpp"

namespace Ludus::Graphics
{
	class OpenGLGraphicsDevice;

    // adapts ImGui platform and renderer calls to the active OpenGL context
    class OpenGLImGuiBackend final : public IImGuiBackend
    {
    public:
        bool SupportsViewports() const override { return true; }
        bool Initialize(GLFWwindow* window, IGraphicsDevice& device) override;
        void BeginFrame() override;
        void Render(ImDrawData* drawData) override;
		ImTextureID AddTexture(GpuTextureHandle texture, GpuSamplerHandle sampler) override;
		void RemoveTexture(ImTextureID texture) override;
        void Shutdown() override;

	private:
		OpenGLGraphicsDevice* _device = nullptr;
    };
}
