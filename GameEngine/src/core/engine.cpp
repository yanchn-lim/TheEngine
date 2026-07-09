#include <cstdio>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "engine.hpp"
#include "core/file_system.hpp"
#include "debug/debug.hpp"
#include "debug/profiler.hpp"

#include "graphics/shader.hpp"
#include "graphics/mesh.hpp"
#include "graphics/texture2d.hpp"
#include "graphics/drawcmd.hpp"
#include "graphics/material.hpp"


static void ProcessInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Global debug/runtime hotkeys are handled at the window callback level.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        Engine::Get().running = false;
    }

    if (key == GLFW_KEY_F5 && action == GLFW_PRESS)
    {
        Profiler::Get().SetPaused(!Profiler::Get().IsPaused());
    }

    if (key == GLFW_KEY_F6 && action == GLFW_PRESS)
    {
        Profiler::Get().PrintFrameStatistics(2048);
        Profiler::Get().PrintFrameStatisticsToFile("FRAME_STATS.txt", 2048);
    }
}

static void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

bool Window::Init()
{
    // Create the OpenGL context before initializing GLAD or renderer resources.
    if (!glfwInit()) return false;

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Debug::CLog("Creating window...\n");
    handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!handle)
    {
        Debug::CLog("Failed to create window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(handle);
    glfwSwapInterval(vsync ? 1 : 0);

    Debug::CLog("Initializing GLAD...\n");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        Debug::CLog("Failed to initialize GLAD\n");
        return false;
    }

    //set debug
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    //glDebugMessageCallback();

    glfwSetFramebufferSizeCallback(handle, [](GLFWwindow*, int w, int h)
        {
            Engine::Get().window.width = w;
            Engine::Get().window.height = h;
            glViewport(0, 0, w, h);
        });

    glfwSetKeyCallback(handle, ProcessInput);

    Debug::CLog("Window created successfully\n");
    return true;
}

void Window::Shutdown()
{
    glfwDestroyWindow(handle);
    glfwTerminate();
}


// ========= IMGUI ==========
bool ImGuiLayer::Init(GLFWwindow* window)
{
    // ImGui is configured once against the active GLFW/OpenGL context.
    Debug::CLog("Initializing ImGui...\n");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
    {
        Debug::CLog("Failed to initialize ImGui GLFW backend\n");
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 460"))
    {
        Debug::CLog("Failed to initialize ImGui OpenGL backend\n");
        return false;
    }

    Debug::CLog("ImGui initialized successfully\n");
    return true;
}

void ImGuiLayer::Begin()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::End()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup);
    }
}

void ImGuiLayer::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}



// ========= ENGINE =========

bool ManualRenderTest::Initialize(Assets::AssetManager& assets)
{
    // Keep temporary visual checks contained while backend and asset APIs settle.
    Assets::ShaderHandle shader = assets.LoadShader("assets/shaders/bugatti.vert", "assets/shaders/bugatti.frag");
    if (!shader)
        return false;

    Assets::TextureHandle texture = assets.LoadTexture("assets/textures/maxwell.png");
    if (!texture)
        return false;

    Graphics::RenderState state = { true, true, Graphics::BlendMode::NONE, true };
    material = assets.CreateMaterial("manual_model_test", shader, texture, state);
    if (!material)
        return false;

    model = assets.LoadModel("manual_model_test", "assets/models/maxwell.obj");
    if (!model)
        return false;

    return true;
}

void ManualRenderTest::Submit(Graphics::Renderer& renderer, const Assets::AssetManager& assets, double deltaTime)
{
    const Assets::ModelAsset* modelAsset = assets.Get(model);
    const Graphics::Material* modelMaterial = assets.Get(material);

    if (!modelAsset || !modelMaterial)
        return;

    for (const Assets::MeshHandle mesh : modelAsset->meshes)
    {
        Graphics::DrawCmd cmd;
        cmd.mesh = assets.Get(mesh);
        cmd.material = modelMaterial;
        cmd.transform = glm::translate(glm::mat4(1.f), glm::vec3(0.f, -0.1f, 0.f))
            * glm::scale(glm::mat4(1.f), glm::vec3(0.002f))
            * glm::rotate(glm::mat4(1.f), rotation, glm::vec3(0.f, 1.f, 0.f));

        renderer.Submit(cmd);
    }

    rotation += 2.0f * static_cast<float>(deltaTime);
}

int Engine::Run()
{
    // Keep the public entrypoint small: initialize, run the frame loop, then teardown.
    if (!Initialize())
        return EXIT_FAILURE;

    Update();
    Shutdown();

    return EXIT_SUCCESS;
}

bool Engine::Initialize()
{
    // Initialization order matters: window/context, UI, renderer, then manual test assets.
    Debug::CLog("========== Initializing engine... ==========\n");
    if (!window.Init())
    {
        Debug::CLog("Failed to initialize window\n");
        return false;
    }

    if (!imgui.Init(window.handle))
    {
        Debug::CLog("Failed to initialize ImGui\n");
        return false;
    }

    if (!renderer.Init())
    {
        Debug::CLog("Failed to initialize renderer\n");
        return false;
    }

    running = true;


    Debug::CLog("Initializing manual render test...\n");
    if (!manualRenderTest.Initialize(assets))
        return false;
     
    Debug::CLog("Successfully initialized manual render test!\n");


    Debug::CLog("========== Initialization Success! ==========\n\n");
    return true;
}

void Engine::Update()
{
    //set time
    time.totalTime = glfwGetTime();
    time.deltaTime = 0.0;

    // Main loop owns per-frame polling, camera setup, rendering, UI, and swap.
    while (!glfwWindowShouldClose(window.handle) && running)
    {
        Profiler::Get().BeginFrame();
        double currentTime = glfwGetTime();
        time.deltaTime = currentTime - time.totalTime;
        time.totalTime = currentTime;

        {
            PROFILE_SCOPE("MainLoop");
            glfwPollEvents();
   
            Graphics::Camera2D camera;
            camera.position = { 0.f, 0.f };
            camera.zoom = 1.f;
            camera.rotation = 0.f;
            camera.SetViewport((float)window.width, (float)window.height);

            renderer.SetCamera(camera);
        }

        {
            PROFILE_SCOPE("RendererLoop");
            renderer.BeginFrame();

            manualRenderTest.Submit(renderer, assets, time.deltaTime);

            renderer.EndFrame();

            {
                PROFILE_SCOPE("ImGui");
                imgui.Begin();
                ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
                profilerUI.Draw();
                DebugConsole::Get().Draw();
                imgui.End();
            }

            glfwSwapBuffers(window.handle);
        }

        Profiler::Get().EndFrame();
    }
}

void Engine::Shutdown()
{
    // Tear down GL-backed systems before destroying the window/context.
    Debug::CLog("========== Shutting down engine... ==========\n");

    renderer.Shutdown();
    assets.Clear();
    imgui.Shutdown();
    window.Shutdown();

    Debug::CLog("Engine shutdown complete\n");
}
