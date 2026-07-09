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
#include "assets/primitives/primitive_mesh2d.hpp"


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

//test assets
Graphics::Mesh testLineMesh;
Assets::MeshHandle testQuadMesh;
Assets::MeshHandle testObjMesh;
Assets::MeshHandle testObjQuadMesh;
std::vector<Assets::MeshHandle> testObjModelMeshes;
Assets::ModelHandle testModel;

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
    // Initialization order matters: window/context, UI, renderer, then test assets.
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


    Assets::ShaderHandle shaderHandle = assets.LoadShader("assets/shaders/sprite.vert", "assets/shaders/sprite.frag");
    Assets::ShaderHandle bshaderHandle = assets.LoadShader("assets/shaders/bugatti.vert", "assets/shaders/bugatti.frag");

    if (!shaderHandle)
        return false;
   

    //testQuadMesh = assets.LoadModel("test_quad_mesh", "assets/models/bugatti.obj");
    testQuadMesh = assets.CreateMesh("test_quad_mesh", Assets::Primitive2D::Quad());
    if (!testQuadMesh)
        return false;


    //testObjMesh = assets.LoadMesh("test_obj_triangle", "assets/models/test_triangle.obj");
    //if (!testObjMesh)
    //    return false;

    //testObjQuadMesh = assets.LoadMesh("test_obj_quad", "assets/models/test_quad.obj");
    //if (!testObjQuadMesh)
    //    return false;

    //testObjModelMeshes = assets.LoadModel("test_obj_two_shapes", "assets/models/test_two_shapes.obj");
    //if (testObjModelMeshes.size() != 2)
    //    return false;

    testModel = assets.LoadModel("testModel", "assets/models/maxwell.obj");
    if (!testModel)
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

    Graphics::MeshUploadData lineQuad{};
    lineQuad.vertices = lineQuadVertices;
    lineQuad.vertexCount = 4;
    lineQuad.indices = lineQuadIndices;
    lineQuad.indexCount = 8;
    lineQuad.layout = Assets::CreateMeshVertexLayout();
    lineQuad.topology = Graphics::PrimitiveTopology::LINES;

    //if (!testLineMesh.Create(lineQuad))
        //return false;

    //load texture
    Assets::TextureHandle steakTexture = assets.LoadTexture("assets/textures/steak.png");
    if (!steakTexture)
        return false;
    Assets::TextureHandle maxwellTexture = assets.LoadTexture("assets/textures/maxwell.png");

    Graphics::RenderState state = { false, false, Graphics::BlendMode::ALPHA, false };
    Graphics::RenderState stateNoBlend = { false, false, Graphics::BlendMode::NONE, false };

    Assets::MaterialHandle testMat = assets.CreateMaterial("steak", shaderHandle, steakTexture, state);
    Assets::MaterialHandle testMatNoBlend = assets.CreateMaterial("steak_noblend", shaderHandle, steakTexture, stateNoBlend);

    Assets::ShaderHandle missingShader = assets.LoadShader("assets/shaders/missing.vert", "assets/shaders/missing.frag");
    Assets::TextureHandle missingTexture = assets.LoadTexture("assets/textures/missing.png");
    Assets::MaterialHandle fallbackMat = assets.CreateMaterial("fallback_debug", missingShader, missingTexture, state);

    Graphics::RenderState stateAdditive = { true, true, Graphics::BlendMode::NONE, true };
    Assets::MaterialHandle btestMat = assets.CreateMaterial("bugatti", bshaderHandle, maxwellTexture, stateAdditive);

    //if (!fallbackMat)
        //return false;
     
    Debug::CLog("Successfully initialized test assets!\n");


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

            Graphics::DrawCmd leftCmd;
            leftCmd.mesh = assets.Get(testQuadMesh);
            leftCmd.material = assets.Get("bugatti");
            leftCmd.transform = glm::scale(glm::mat4(1.f), glm::vec3(0.5f) );
            
            //Graphics::DrawCmd rightCmd;
            //rightCmd.mesh = assets.Get(testQuadMesh);
            //rightCmd.material = assets.Get("steak_noblend");
            //rightCmd.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.35f, 0.0f, 0.0f));

            //Graphics::DrawCmd lineCmd;
            //lineCmd.mesh = &testLineMesh;
            //lineCmd.material = assets.Get("steak");
            //lineCmd.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.35f, 0.0f));

            //Graphics::DrawCmd fallbackCmd;
            //fallbackCmd.mesh = assets.Get(testQuadMesh);
            //fallbackCmd.material = assets.Get("fallback_debug");
            //fallbackCmd.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.45f, 0.0f));

            //Graphics::DrawCmd objCmd;
            //objCmd.mesh = assets.Get(testObjMesh);
            //objCmd.material = assets.Get("steak");
            //objCmd.transform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.35f, 0.65f, 0.0f));

            //Graphics::DrawCmd objQuadCmd;
            //objQuadCmd.mesh = assets.Get(testObjQuadMesh);
            //objQuadCmd.material = assets.Get("steak");
            //objQuadCmd.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.35f, 0.65f, 0.0f));

            //Graphics::DrawCmd modelShape0Cmd;
            //modelShape0Cmd.mesh = assets.Get(testObjModelMeshes[0]);
            //modelShape0Cmd.material = assets.Get("steak");
            //modelShape0Cmd.transform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.35f, -0.75f, 0.0f));

            //Graphics::DrawCmd modelShape1Cmd;
            //modelShape1Cmd.mesh = assets.Get(testObjModelMeshes[1]);
            //modelShape1Cmd.material = assets.Get("steak");
            //modelShape1Cmd.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.35f, -0.75f, 0.0f));

            //renderer.Submit(leftCmd);
            //renderer.Submit(rightCmd);
            //renderer.Submit(lineCmd);
            //renderer.Submit(fallbackCmd);
            //renderer.Submit(objCmd);
            //renderer.Submit(objQuadCmd);
            //renderer.Submit(modelShape0Cmd);
            //renderer.Submit(modelShape1Cmd);
            static glm::float32 rot = 0.f;

            for (const auto& mesh : assets.Get(testModel)->meshes)
            {
                Graphics::DrawCmd modelShapeCmd;
                modelShapeCmd.mesh = assets.Get(mesh);
                modelShapeCmd.material = assets.Get("bugatti");
                modelShapeCmd.transform = glm::translate(glm::mat4(1.f), glm::vec3(0.f,-0.1f,0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(0.002f)) * glm::rotate(glm::mat4(1.f), rot, glm::vec3(0.f, 1.f, 0.f));
				renderer.Submit(modelShapeCmd);
            }
            
            rot += 2 * static_cast<glm::float32>(time.deltaTime);

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
    imgui.Shutdown();
    window.Shutdown();

    testLineMesh.Destroy();

    assets.Clear();

    Debug::CLog("Engine shutdown complete\n");
}
