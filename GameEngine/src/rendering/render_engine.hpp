#pragma once

#include <memory>
#include <vector>

#include "graphics/camera.hpp"
#include "graphics/graphics_device.hpp"
#include "graphics/imgui_layer.hpp"
#include "graphics/renderer_backend.hpp"
#include "render_resource_manager.hpp"
#include "render_item.hpp"

namespace Ludus::Rendering
{
    // owns the graphics device, imgui integration, and renderer gpu caches.
    // borrows AssetManager through RenderResourceManager.
    // a successful Render call must be followed by EndFrame.
    class RenderEngine
    {
    public:
        static std::unique_ptr<RenderEngine> Create(
            Ludus::Graphics::RendererBackend backend,
            const Ludus::Graphics::GraphicsDeviceDesc& deviceDesc,
            const Ludus::Assets::AssetManager& assets);

        RenderEngine(std::unique_ptr<Ludus::Graphics::IGraphicsDevice> device, const Ludus::Assets::AssetManager& assets);
        ~RenderEngine();
        void BeginImGui();
        void EndImGui();
        void Submit(MeshInstanceDesc item);
		void RequestEditorViewportSize(uint32_t width, uint32_t height);
		ImTextureID GetEditorViewportTexture() const noexcept;
		bool EditorViewportNeedsVerticalFlip() const noexcept;
        // Render records scene draws and leaves the window pass open for ui.
        // EndFrame closes that pass, clears submissions, and presents the frame.
        Ludus::Graphics::FrameStatus Render(const Ludus::Graphics::Camera& camera, uint32_t width, uint32_t height);
        Ludus::Graphics::FrameStatus EndFrame();
        void OnResize(uint32_t width, uint32_t height);
        void Shutdown();

    private:
        std::unique_ptr<Ludus::Graphics::IGraphicsDevice> _device;
        // resource resolution and frame-local items stay private to the renderer
        RenderResourceManager _resources;
        Ludus::Graphics::ImGuiLayer _imgui;
        std::vector<MeshInstanceDesc> _items;
        bool _shutdown = false;
		Ludus::Graphics::RendererBackend _backend;
		Ludus::Graphics::GpuRenderTargetHandle _editorTarget;
		Ludus::Graphics::GpuSamplerHandle _editorSampler;
		ImTextureID _editorTexture = ImTextureID_Invalid;
		uint32_t _editorWidth = 0;
		uint32_t _editorHeight = 0;
		uint32_t _requestedEditorWidth = 0;
		uint32_t _requestedEditorHeight = 0;

        void Draw(const MeshInstanceDesc& item, const Ludus::Graphics::FrameConstants& frameConstants);
		void ApplyEditorViewportSize();
    };
}
