#include "engine.hpp"

#include <cstdlib>
#include <utility>

#include "application.hpp"
#include "debug/debug.hpp"
#include "debug/memory_tracker.hpp"
#include "debug/profiler.hpp"
#include "rendering/render_engine.hpp"

namespace Ludus
{
Engine::Engine(EngineConfig config)
    : _config(std::move(config))
{
}

Engine::~Engine() = default;

int Engine::Run(IApplication& application)
{
    _application = &application;

    if (!Initialize())
    {
        Shutdown();
        _application = nullptr;
        return EXIT_FAILURE;
    }

    if (!_application->OnInitialize(*this))
    {
        _application->OnShutdown(*this);
        Shutdown();
        _application = nullptr;
        return EXIT_FAILURE;
    }

    Update();
    _application->OnShutdown(*this);
    Shutdown();
    _application = nullptr;
    return EXIT_SUCCESS;
}

void Engine::RequestStop() noexcept
{
    _running = false;
}

const Time& Engine::GetTime() const noexcept
{
    return _time;
}

Ludus::Assets::AssetManager& Engine::GetAssets() noexcept
{
    return _assets;
}

Ludus::Rendering::RenderEngine& Engine::GetRenderEngine() noexcept
{
    return *_renderEngine;
}

const Input& Engine::GetInput() const noexcept
{
	return _input;
}

bool Engine::Initialize()
{
    if (_config.fixedTimeStep <= 0.0)
        return false;

	_window.SetInput(&_input);
    if (!_window.Initialize(
        _config.rendererBackend,
        _config.windowWidth,
        _config.windowHeight,
        _config.windowTitle.c_str()))
    {
        return false;
    }

    _renderEngine = Ludus::Rendering::RenderEngine::Create(
        _config.rendererBackend,
        { _window.GetNativeHandle(), _config.vsync },
        _assets);

    if (!_renderEngine)
        return false;

    _renderEngine->OnResize(
        static_cast<uint32_t>(_window.GetWidth()),
        static_cast<uint32_t>(_window.GetHeight()));

    _running = true;
    return true;
}

void Engine::Update()
{
    _time.totalTime = _window.GetTime();
    double fixedAccumulator = 0.0;

    while (!_window.ShouldClose() && _running)
    {
        Profiler::Get().BeginFrame();
        Ludus::Memory::BeginFrame();
        {
            PROFILE_SCOPE("MainLoop");

            {
                PROFILE_SCOPE("Platform Events");
                const double current = _window.GetTime();
                _time.deltaTime = current - _time.totalTime;
                _time.totalTime = current;
				_input.BeginFrame();
                _window.PollEvents();

                fixedAccumulator += _time.deltaTime;
                while (fixedAccumulator >= _config.fixedTimeStep)
                {
                    _application->OnFixedUpdate(*this, _config.fixedTimeStep);
                    fixedAccumulator -= _config.fixedTimeStep;
                }

                if (_window.IsResizePending())
                {
                    _renderEngine->OnResize(
                        static_cast<uint32_t>(_window.GetWidth()),
                        static_cast<uint32_t>(_window.GetHeight()));
                    _window.ClearResizePending();
                }
            }

            {
                PROFILE_SCOPE("Simulation");
                _application->OnUpdate(*this);
            }
        }

        {
            PROFILE_SCOPE("RendererLoop");
            Ludus::Graphics::FrameStatus frameStatus = Ludus::Graphics::FrameStatus::Skip;
            {
                PROFILE_SCOPE("World Rendering");
                Ludus::Graphics::Camera2D camera;
                camera.SetViewport(
                    static_cast<float>(_window.GetWidth()),
                    static_cast<float>(_window.GetHeight()));
                frameStatus = _renderEngine->Render(
                    camera,
                    static_cast<uint32_t>(_window.GetWidth()),
                    static_cast<uint32_t>(_window.GetHeight()));
            }

            if (frameStatus == Ludus::Graphics::FrameStatus::Success)
            {
                PROFILE_SCOPE("ImGui");
                _renderEngine->BeginImGui();
                _application->OnImGui(*this);
                _renderEngine->EndImGui();
            }

            if (frameStatus == Ludus::Graphics::FrameStatus::Success)
            {
                PROFILE_SCOPE("Frame Completion");
                frameStatus = _renderEngine->EndFrame();
            }

            if (frameStatus == Ludus::Graphics::FrameStatus::DeviceLost ||
                frameStatus == Ludus::Graphics::FrameStatus::Fatal)
            {
                Ludus::Debug::LogError("graphics frame failed with status ", static_cast<int>(frameStatus));
                RequestStop();
            }
        }
        Ludus::Memory::EndFrame();
        Profiler::Get().EndFrame();
    }
}

void Engine::Shutdown()
{
    if (_renderEngine)
        _renderEngine->Shutdown();
    _assets.Clear();
    _renderEngine.reset();
	_window.SetInput(nullptr);
    _window.Shutdown();
    _running = false;
}

}
