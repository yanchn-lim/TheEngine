#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "engine.hpp"
#include "debug.hpp"
#include "profiler.hpp"

#include "renderer.hpp"
#include "graphics_resource.hpp"
#include "material.hpp"


static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // let ImGui consume scroll if it wants it
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    Engine::Get().camera.ProcessZoom((float)yoffset);
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        Engine::Get().input.middleMouseHeld = (action == GLFW_PRESS);

        if (action == GLFW_PRESS)
        {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            Engine::Get().input.lastMousePos = float2((float)x, (float)y);
        }
    }
}

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

    //build a map of keys to a function ptr?
    //set map layers to change on the fly
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
    handle = glfwCreateWindow(size.x, size.y, title, nullptr, nullptr);
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

    // Keep size in sync if the user resizes
    glfwSetFramebufferSizeCallback(handle, [](GLFWwindow*, int w, int h)
        {
            Engine::Get().window.size = { w, h };
            glViewport(0, 0, w, h);
        });

    glfwSetKeyCallback(handle, ProcessInput);
    glfwSetScrollCallback(handle, ScrollCallback);
    glfwSetMouseButtonCallback(handle, MouseButtonCallback);

    Debug::CLog("Window created successfully\n");
    return true;
}

void Window::Shutdown()
{
    glfwDestroyWindow(handle);
    glfwTerminate();
}

bool ImGuiLayer::Init(GLFWwindow* window)
{
    Debug::CLog("Initializing ImGui...\n");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

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

    // Multi-viewport support — render ImGui windows outside the main window
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

//ENGINE
int Engine::Run()
{
    if (!Initialize()) 
        return -1;

    Update();
    Shutdown();

    return 0;
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

    running = true;

    
    if (!Graphics::Renderer::Get().Init())
    {
        Debug::CLog("Failed to initialize Renderer\n");
        return false;
    }

    Graphics::Resource::Initialize();
    Graphics::Resource::MaterialLibrary::Get().Add("unlit_mat", {"unlit","wall_brick",0});

    Debug::CLog("========== Initialization Success! ==========\n\n");


    return true;
}

void Engine::Update()
{
    int frameCount = 0;
    while (!glfwWindowShouldClose(window.handle) && running)
    {
        Profiler::Get().BeginFrame();

        {
            PROFILE_SCOPE("MainLoop");
            glfwPollEvents();


            // --- Begin frame ---
            //clear buffers
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // [game update]
            {
                PROFILE_SCOPE("Update");
                //Debug::Log("Frames passed : ",frameCount++);

                if (Engine::Get().input.middleMouseHeld)
                {
                    double x, y;
                    glfwGetCursorPos(window.handle, &x, &y);
                    float2 currentPos = float2((float)x, (float)y);
                    float2 delta = input.lastMousePos - currentPos; // inverted: drag right moves camera right
                    input.lastMousePos = currentPos;
                    camera.ProcessPan(delta);
                }
            }

            // [render]
            {
                using namespace Graphics;
                PROFILE_SCOPE("Render");
                Renderer::Get().Begin();

                Renderer::Get().SetCamera(camera.GetViewProjection(window.size));

                {
                    PROFILE_SCOPE("QUEUE");
                    for (int x = 0; x < 100; x++)
                    {
                        for (int y = 0; y < 100; y++)
                        {
                            DrawCommand cmd{ "unlit_mat", "quad", DrawCommand::SetModel(float3(x * 200.f, y * 200.f, 0.f), float3(100.f, 100.f, 1.f), 0.f) };
                            Renderer::Get().Queue(cmd);
                        }
                    }
                }


                Renderer::Get().End();
            }

            {         
                PROFILE_SCOPE("ImGui");
                imgui.Begin();
                //set a dockspace to the entire viewport
                ImGui::DockSpaceOverViewport(0,ImGui::GetMainViewport(),ImGuiDockNodeFlags_PassthruCentralNode);
                //draw ui
                profilerUI.Draw();
                DebugConsole::Get().Draw();
                //ImGui::ShowDemoWindow();

                imgui.End();
            }
            // --- End frame ---
            glfwSwapBuffers(window.handle);
        }

        Profiler::Get().EndFrame();
    }
}

void Engine::Shutdown()
{
    Debug::CLog("========== Shutting down engine... ==========\n");
    imgui.Shutdown();
    window.Shutdown();
    Graphics::Resource::Shutdown();
    Debug::CLog("Engine shutdown complete\n");

    Graphics::Renderer::Get().Shutdown();
}