#include "engine.hpp"

#include <cstdio>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include "assets/primitives/primitive_mesh2d.hpp"
#include "debug/debug.hpp"
#include "debug/memory_tracker.hpp"
#include "debug/profiler.hpp"
#include "graphics/graphics_device_factory.hpp"
#include "tests/graphics_api_tests.hpp"

namespace
{
    void ProcessInput(GLFWwindow*, int key, int, int action, int)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) Engine::Get().running = false;
        if (key == GLFW_KEY_F5 && action == GLFW_PRESS) Profiler::Get().SetPaused(!Profiler::Get().IsPaused());
        if (key == GLFW_KEY_F6 && action == GLFW_PRESS)
        {
            Profiler::Get().PrintFrameStatistics(2048);
            Profiler::Get().PrintFrameStatisticsToFile("FRAME_STATS.txt", 2048);
        }
    }

    void ErrorCallback(int error, const char* description)
    {
        std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    }
}

Engine& Engine::Get()
{
    static Engine engine;
    return engine;
}

bool Window::Init(Graphics::RendererBackend backend)
{
    if (!glfwInit()) return false;
    glfwSetErrorCallback(ErrorCallback);
    if (backend == Graphics::RendererBackend::OPENGL)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }
    else
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!handle) { glfwTerminate(); return false; }
    glfwSetFramebufferSizeCallback(handle, [](GLFWwindow*, int w, int h)
    {
        Engine::Get().window.width = w;
        Engine::Get().window.height = h;
        Engine::Get().window.resizePending = true;
    });
    glfwSetKeyCallback(handle, ProcessInput);
    return true;
}

void Window::Shutdown()
{
    if (handle) glfwDestroyWindow(handle);
    handle = nullptr;
    glfwTerminate();
}

bool ImGuiLayer::Init(GLFWwindow* window, Graphics::RendererBackend backend, Graphics::IGraphicsDevice& device)
{
    _backend = Graphics::CreateImGuiBackend(backend);
    if (!_backend || !_backend->IsAvailable()) return true;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (_backend->SupportsViewports()) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();
    if (!_backend->Initialize(window, device))
    {
        ImGui::DestroyContext();
        _backend.reset();
        return false;
    }
    _initialized = true;
    return true;
}

void ImGuiLayer::Begin()
{
    _backend->BeginFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::End()
{
    ImGui::Render();
    _backend->Render(ImGui::GetDrawData());
}

void ImGuiLayer::Shutdown()
{
    if (_initialized)
    {
        _backend->Shutdown();
        ImGui::DestroyContext();
        _initialized = false;
    }
    _backend.reset();
}

bool ManualRenderTest::Initialize(Assets::AssetManager& assetManager, Rendering::RenderWorld& world)
{
    const Assets::ShaderHandle shader = assetManager.LoadShader(
        "assets/shaders/standard_gl.vert", "assets/shaders/standard_gl.frag",
        "assets/shaders/standard_vk.vert.spv", "assets/shaders/standard_vk.frag.spv");
    const Assets::TextureHandle texture = assetManager.LoadTexture("assets/textures/maxwell.png");
    if (!shader || !texture) return false;
    material = assetManager.CreateMaterial("standard_material", shader, texture,
        { false, false, Graphics::BlendMode::NONE, true });
    model = assetManager.LoadModel("manual_model", "assets/models/maxwell.obj");
    spriteMesh = assetManager.CreateMesh("builtin_quad", Assets::Primitive2D::Quad());
    const Assets::ModelAsset* modelAsset = assetManager.Get(model);
    if (!material || !modelAsset || !spriteMesh) return false;
    for (Assets::MeshHandle mesh : modelAsset->meshes)
        instances.push_back(world.CreateMeshInstance({ mesh, material }));
    return true;
}

void ManualRenderTest::Update(Rendering::RenderWorld& world, double deltaTime)
{
    const glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.002f)) *
        glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 1.0f, 0.0f));
    for (Rendering::RenderInstanceHandle instance : instances) world.SetTransform(instance, transform);
    rotation += 2.0f * static_cast<float>(deltaTime);
}

void ManualRenderTest::Shutdown(Rendering::RenderWorld& world)
{
    for (Rendering::RenderInstanceHandle instance : instances) world.Destroy(instance);
    instances.clear();
}

int Engine::Run()
{
    if (!Initialize())
    {
        Shutdown();
        return EXIT_FAILURE;
    }
    Update();
    Shutdown();
    return EXIT_SUCCESS;
}

bool Engine::Initialize()
{
#ifndef NDEBUG
    if (!Tests::RunGraphicsApiTests())
    {
        Debug::LogError("Graphics API self-tests failed");
        return false;
    }
#endif
    if (!window.Init(renderbackend)) return false;
    graphicsDevice = Graphics::CreateGraphicsDevice(renderbackend);
    if (!graphicsDevice || !graphicsDevice->Initialize({ window.handle, window.vsync })) return false;
    renderer = std::make_unique<Rendering::Renderer>(*graphicsDevice, assets, renderWorld);
    if (!manualRenderTest.Initialize(assets, renderWorld)) return false;
    renderer->Configure({ manualRenderTest.spriteMesh, manualRenderTest.material });
    if (!imgui.Init(window.handle, renderbackend, *graphicsDevice)) return false;
    renderer->OnResize(static_cast<uint32_t>(window.width), static_cast<uint32_t>(window.height));
    running = true;
    return true;
}

void Engine::Update()
{
    time.totalTime = glfwGetTime();
    while (!glfwWindowShouldClose(window.handle) && running)
    {
        Profiler::Get().BeginFrame();
        Memory::BeginFrame();
        {
            PROFILE_SCOPE("MainLoop");

            {
                PROFILE_SCOPE("Platform Events");
                const double current = glfwGetTime();
                time.deltaTime = current - time.totalTime;
                time.totalTime = current;
                glfwPollEvents();
                if (window.resizePending)
                {
                    renderer->OnResize(static_cast<uint32_t>(window.width), static_cast<uint32_t>(window.height));
                    window.resizePending = false;
                }
            }

            {
                PROFILE_SCOPE("Simulation");
                manualRenderTest.Update(renderWorld, time.deltaTime);
            }
        }

        {
            PROFILE_SCOPE("RendererLoop");
            Graphics::FrameStatus frameStatus = Graphics::FrameStatus::Skip;
            {
                PROFILE_SCOPE("World Rendering");
                Graphics::Camera2D camera;
                camera.SetViewport(static_cast<float>(window.width), static_cast<float>(window.height));
                frameStatus = renderer->Render(camera,
                    static_cast<uint32_t>(window.width), static_cast<uint32_t>(window.height));
            }

            if (frameStatus == Graphics::FrameStatus::Success && imgui.IsInitialized())
            {
                PROFILE_SCOPE("ImGui");
                imgui.Begin();
                if (ImGui::BeginMainMenuBar())
                {
                    if (ImGui::BeginMenu("Tools"))
                    {
                        ImGui::MenuItem("Profiler", nullptr, &profilerUI.IsOpen());
                        ImGui::MenuItem("Console", nullptr, &DebugConsole::Get().IsOpen());
                        ImGui::EndMenu();
                    }
                    ImGui::EndMainMenuBar();
                }
                ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
                profilerUI.Draw();
                DebugConsole::Get().Draw();
                imgui.End();
            }

            if (frameStatus == Graphics::FrameStatus::Success)
            {
                PROFILE_SCOPE("Command Submission");
                frameStatus = renderer->EndFrame();
            }
            if (frameStatus == Graphics::FrameStatus::Success)
            {
                PROFILE_SCOPE("Presentation");
                frameStatus = renderer->Present();
            }

            if (frameStatus == Graphics::FrameStatus::DeviceLost ||
                frameStatus == Graphics::FrameStatus::Fatal)
            {
                Debug::LogError("graphics frame failed with status ", static_cast<int>(frameStatus));
                running = false;
            }
        }
        Memory::EndFrame();
        Profiler::Get().EndFrame();
    }
}

void Engine::Shutdown()
{
    manualRenderTest.Shutdown(renderWorld);
    imgui.Shutdown();
    if (renderer) renderer->Shutdown();
    renderWorld.Clear();
    assets.Clear();
    if (graphicsDevice) graphicsDevice->Shutdown();
    renderer.reset();
    graphicsDevice.reset();
    window.Shutdown();
}
