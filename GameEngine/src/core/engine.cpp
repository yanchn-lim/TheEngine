#include "engine.hpp"

#include <cstdlib>
#include <utility>

#include "application.hpp"
#include "debug/debug.hpp"
#include "debug/memory_tracker.hpp"
#include "debug/profiler.hpp"
#include "graphics/graphics_device_factory.hpp"
#include "rendering/renderer.hpp"

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

Assets::AssetManager& Engine::GetAssets() noexcept
{
    return _assets;
}

Rendering::RenderWorld& Engine::GetRenderWorld() noexcept
{
    return _renderWorld;
}

Rendering::Renderer& Engine::GetRenderer() noexcept
{
    return *_renderer;
}

bool Engine::Initialize()
{
    if (!_window.Initialize(
        _config.rendererBackend,
        _config.windowWidth,
        _config.windowHeight,
        _config.windowTitle.c_str()))
    {
        return false;
    }

    _window.SetKeyCallback(
        [](void* context, int key, int action)
        {
            static_cast<Engine*>(context)->HandleKey(key, action);
        },
        this);

    _graphicsDevice = Graphics::CreateGraphicsDevice(_config.rendererBackend);
    if (!_graphicsDevice || !_graphicsDevice->Initialize({ _window.GetNativeHandle(), _config.vsync }))
        return false;

    _renderer = std::make_unique<Rendering::Renderer>(*_graphicsDevice, _assets, _renderWorld);

    if (!_imgui.Initialize(_window.GetNativeHandle(), _config.rendererBackend, *_graphicsDevice))
        return false;

    _renderer->OnResize(
        static_cast<uint32_t>(_window.GetWidth()),
        static_cast<uint32_t>(_window.GetHeight()));

    _running = true;
    return true;
}

void Engine::Update()
{
    _time.totalTime = _window.GetTime();
    while (!_window.ShouldClose() && _running)
    {
        Profiler::Get().BeginFrame();
        Memory::BeginFrame();
        {
            PROFILE_SCOPE("MainLoop");

            {
                PROFILE_SCOPE("Platform Events");
                const double current = _window.GetTime();
                _time.deltaTime = current - _time.totalTime;
                _time.totalTime = current;
                _window.PollEvents();

                if (_window.IsResizePending())
                {
                    _renderer->OnResize(
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
            Graphics::FrameStatus frameStatus = Graphics::FrameStatus::Skip;
            {
                PROFILE_SCOPE("World Rendering");
                Graphics::Camera2D camera;
                camera.SetViewport(
                    static_cast<float>(_window.GetWidth()),
                    static_cast<float>(_window.GetHeight()));
                frameStatus = _renderer->Render(
                    camera,
                    static_cast<uint32_t>(_window.GetWidth()),
                    static_cast<uint32_t>(_window.GetHeight()));
            }

            if (frameStatus == Graphics::FrameStatus::Success && _imgui.IsInitialized())
            {
                PROFILE_SCOPE("ImGui");
                _imgui.Begin();
                _application->OnImGui(*this);
                _imgui.End();
            }

            if (frameStatus == Graphics::FrameStatus::Success)
            {
                PROFILE_SCOPE("Command Submission");
                frameStatus = _renderer->EndFrame();
            }
            if (frameStatus == Graphics::FrameStatus::Success)
            {
                PROFILE_SCOPE("Presentation");
                frameStatus = _renderer->Present();
            }

            if (frameStatus == Graphics::FrameStatus::DeviceLost ||
                frameStatus == Graphics::FrameStatus::Fatal)
            {
                Debug::LogError("graphics frame failed with status ", static_cast<int>(frameStatus));
                RequestStop();
            }
        }
        Memory::EndFrame();
        Profiler::Get().EndFrame();
    }
}

void Engine::Shutdown()
{
    _imgui.Shutdown();
    if (_renderer)
        _renderer->Shutdown();
    _renderWorld.Clear();
    _assets.Clear();
    if (_graphicsDevice)
        _graphicsDevice->Shutdown();
    _renderer.reset();
    _graphicsDevice.reset();
    _window.Shutdown();
    _running = false;
}

void Engine::HandleKey(int key, int action)
{
    if (_application)
        _application->OnKey(*this, key, action);
}
}
