#pragma once

#include <memory>

#include "assets/asset_manager.hpp"
#include "engine_config.hpp"
#include "platform/window.hpp"
#include "time.hpp"

namespace Rendering
{
    class RenderEngine;
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
        Rendering::RenderEngine& GetRenderEngine() noexcept;

    private:
        bool Initialize();
        void Update();
        void Shutdown();
        void HandleKey(int key, int action);

        EngineConfig _config;
        Time _time{};
        Platform::Window _window;
        Assets::AssetManager _assets;
        std::unique_ptr<Rendering::RenderEngine> _renderEngine;
        IApplication* _application = nullptr;
        bool _running = false;
    };
}
