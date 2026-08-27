#pragma once

#include <memory>

#include "assets/asset_manager.hpp"
#include "engine_config.hpp"
#include "input.hpp"
#include "platform/window.hpp"
#include "time.hpp"

namespace Ludus::Rendering
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
        Ludus::Assets::AssetManager& GetAssets() noexcept;
        Ludus::Rendering::RenderEngine& GetRenderEngine() noexcept;
		const Input& GetInput() const noexcept;

    private:
        bool Initialize();
        void Update();
        void Shutdown();

        EngineConfig _config;
        Time _time{};
		Input _input;
        Ludus::Platform::Window _window;
        Ludus::Assets::AssetManager _assets;
        std::unique_ptr<Ludus::Rendering::RenderEngine> _renderEngine;
        IApplication* _application = nullptr;
        bool _running = false;
    };
}
