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
#include "graphics/primitive2d.hpp"
#include "graphics/drawcmd.hpp"
#include "graphics/material.hpp"


static void ProcessInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
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

//test assets
Graphics::Mesh testMesh;
Graphics::Mesh testLineMesh;
Graphics::Shader testShader;
Graphics::Texture2D testTexture;
Graphics::Material testMaterial;
Graphics::Material testMaterialNoBlend;

int Engine::Run()
{
    if (!Initialize())
        return EXIT_FAILURE;

    Update();
    Shutdown();

    return EXIT_SUCCESS;
}

bool Engine::Initialize()
{
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


    //init test assets
    Debug::CLog("Initializing test assets...\n");
    std::string vertexSource;
    std::string fragmentSource;

    if (!FileSystem::ReadTextFile("assets/shaders/sprite.vert", vertexSource))
    {
        Debug::LogError("Failed to read vertex shader");
        return false;
    }

    if (!FileSystem::ReadTextFile("assets/shaders/sprite.frag", fragmentSource))
    {
        Debug::LogError("Failed to read fragment shader");
        return false;
    }

    if (!testShader.Create(vertexSource, fragmentSource))
        return false;

    if (!testShader.IsValid()) 
        return false;

    const Graphics::MeshData quad = Graphics::Primitive2D::Quad();
    if (!testMesh.Create(quad)) 
        return false;

    constexpr uint32_t lineQuadIndices[] =
    {
        0, 1,
        1, 2,
        2, 3,
        3, 0
    };

    constexpr float lineQuadVertices[] =
    {
        // position          // color           // uv
        -0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f, 0.5f, 0.5f,
         0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f, 0.5f, 0.5f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f, 0.5f, 0.5f,
        -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f, 0.5f, 0.5f
    };

    Graphics::MeshData lineQuad{};
    lineQuad.vertices = lineQuadVertices;
    lineQuad.vertexCount = 4;
    lineQuad.indices = lineQuadIndices;
    lineQuad.indexCount = 8;
    lineQuad.layout = quad.layout;
    lineQuad.topology = Graphics::PrimitiveTopology::LINES;

    if (!testLineMesh.Create(lineQuad))
        return false;

    //load texture
    if (!testTexture.LoadFromFile("assets/textures/steak.png"))
        return false;

    //test material
    testMaterial.shader = &testShader;
    testMaterial.texture = &testTexture;
    testMaterial.state.blendMode = Graphics::BlendMode::ALPHA;
    testMaterial.state.depthTest = false;

    testMaterialNoBlend.shader = &testShader;
    testMaterialNoBlend.texture = &testTexture;
    testMaterialNoBlend.state.blendMode = Graphics::BlendMode::NONE;
    testMaterialNoBlend.state.depthTest = false;


    Debug::CLog("Successfully initialized test assets!\n");

    Debug::CLog("========== Initialization Success! ==========\n\n");
    return true;
}

void Engine::Update()
{
    while (!glfwWindowShouldClose(window.handle) && running)
    {
        Profiler::Get().BeginFrame();

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

            Graphics::DrawCmd leftCmd;
            leftCmd.mesh = &testMesh;
            leftCmd.material = &testMaterial;
            leftCmd.transform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.35f, 0.0f, 0.0f));

            Graphics::DrawCmd rightCmd;
            rightCmd.mesh = &testMesh;
            rightCmd.material = &testMaterialNoBlend;
            rightCmd.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.35f, 0.0f, 0.0f));

            Graphics::DrawCmd lineCmd;
            lineCmd.mesh = &testLineMesh;
            lineCmd.material = &testMaterial;
            lineCmd.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.35f, 0.0f));

            renderer.Submit(leftCmd);
            renderer.Submit(rightCmd);
            renderer.Submit(lineCmd);

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
    Debug::CLog("========== Shutting down engine... ==========\n");

    renderer.Shutdown();
    imgui.Shutdown();
    window.Shutdown();


    testTexture.Destroy();
    testShader.Destroy();
    testLineMesh.Destroy();
    testMesh.Destroy();

    Debug::CLog("Engine shutdown complete\n");
}
