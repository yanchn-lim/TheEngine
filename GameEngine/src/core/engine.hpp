#pragma once

#include <memory>
#include <vector>

#include "assets/asset_manager.hpp"
#include "debug/profiler_ui.hpp"
#include "graphics/graphics_device.hpp"
#include "graphics/imgui_backend.hpp"
#include "graphics/renderer_backend.hpp"
#include "rendering/renderer.hpp"
#include "rendering/render_world.hpp"
#include "time.hpp"

struct GLFWwindow;

struct Window
{
    GLFWwindow* handle = nullptr;
    int width = 1600;
    int height = 900;
    const char* title = "Engine";
    bool vsync = false;
    bool resizePending = false;

    bool Init(Graphics::RendererBackend backend);
    void Shutdown();
};

struct ImGuiLayer
{
    bool Init(GLFWwindow* window, Graphics::RendererBackend backend, Graphics::IGraphicsDevice& device);
    void Begin();
    void End();
    void Shutdown();
    bool IsInitialized() const { return _initialized; }

private:
    bool _initialized = false;
    std::unique_ptr<Graphics::IImGuiBackend> _backend;
};

struct ManualRenderTest
{
    Assets::ModelHandle model;
    Assets::MaterialHandle material;
    Assets::MeshHandle spriteMesh;
    std::vector<Rendering::RenderInstanceHandle> instances;
    float rotation = 0.0f;

    bool Initialize(Assets::AssetManager& assets, Rendering::RenderWorld& renderWorld);
    void Update(Rendering::RenderWorld& renderWorld, double deltaTime);
    void Shutdown(Rendering::RenderWorld& renderWorld);
};

class Engine
{
public:
    static Engine& Get();
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    int Run();

    Time time;
    Window window;
    ImGuiLayer imgui;
    ProfilerUI profilerUI;
    //Graphics::RendererBackend renderbackend = Graphics::RendererBackend::OPENGL;
    Graphics::RendererBackend renderbackend = Graphics::RendererBackend::VULKAN;

    Assets::AssetManager assets;
    Rendering::RenderWorld renderWorld;
    std::unique_ptr<Graphics::IGraphicsDevice> graphicsDevice;
    std::unique_ptr<Rendering::Renderer> renderer;
    ManualRenderTest manualRenderTest;
    bool running = false;

private:
    Engine() = default;
    bool Initialize();
    void Update();
    void Shutdown();
};
