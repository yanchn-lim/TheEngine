#pragma once

#include <memory>

#include "assets/asset_manager.hpp"
#include "engine_config.hpp"
#include "graphics/imgui_layer.hpp"
#include "platform/window.hpp"
#include "rendering/render_world.hpp"
#include "time.hpp"

namespace Graphics
{
    class IGraphicsDevice;
}

namespace Rendering
{
    class Renderer;
}

namespace Ludus
{
    class IApplication;

    class Engine
    {
    public:
        explicit Engine(EngineConfig config);
        ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;
        Engine(Engine&&) = delete;
        Engine& operator=(Engine&&) = delete;

        int Run(IApplication& application);
        void RequestStop() noexcept;

        const Time& GetTime() const noexcept;
        Assets::AssetManager& GetAssets() noexcept;
        Rendering::RenderWorld& GetRenderWorld() noexcept;
        Rendering::Renderer& GetRenderer() noexcept;

    private:
        bool Initialize();
        void Update();
        void Shutdown();
        void HandleKey(int key, int action);

        EngineConfig _config;
        Time _time{};
        Platform::Window _window;
        Graphics::ImGuiLayer _imgui;
        Assets::AssetManager _assets;
        Rendering::RenderWorld _renderWorld;
        std::unique_ptr<Graphics::IGraphicsDevice> _graphicsDevice;
        std::unique_ptr<Rendering::Renderer> _renderer;
        IApplication* _application = nullptr;
        bool _running = false;
    };
}
