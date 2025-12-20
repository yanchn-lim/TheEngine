#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

#include "Core/time.hpp"
#include "events.hpp"
#include "Input/input.hpp"
#include "ECS/ecs.hpp"
#include "ECS/components.hpp"
#include "Rendering/render_system.hpp"

#include "Core/debug.hpp"
#include "Core/debug_system.hpp"

/*=====GENERAL NAMING CONVENTIONS=====
    VARIABLES :
        - m_foo -> member variable (non-static)
        - g_foo -> global variable
        - s_foo -> static variable (member or function-local)
        - kFoo  -> constant value

    FUNCTION / Types  :
        - names in pamel case [GetThing() or PlayerController]
*/
static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

GLFWwindow* InitializeGLFW()
{
    // initializing glfw
    glfwInit(); // setting up internal stuff to interface with windows
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // telling glfw that i want v4
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5); // specifically v4.5
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // forces VAOs,shaders,etc
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // creating the window
    GLFWwindow* window = glfwCreateWindow(1600, 900, "The Engine", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(window);

    // loading opengl functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        exit(-1);
    }

    // viewport and resize
    glViewport(0, 0, 1600, 900);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    return window;
}

void MainLoop(GLFWwindow* window)
{
   
}

int main()
{
    // creating the window
    GLFWwindow* window = InitializeGLFW();

    //pretty much singletons here
    //initializing global systems
    Engine::Time       g_Time; 
    Engine::TimeConfig g_TimeConfig;
    Engine::EventBus   g_EventBus; //create a global event bus
    Engine::ECS::World g_World;
    bool m_Running = true;

    //initializing systems
    double startTime = glfwGetTime();
    Engine::TimeSystem::Initialize(g_Time, startTime, g_TimeConfig);
    Engine::InputSystem::Initialize(window, &g_EventBus);

    // plug in debug frame listener
    Engine::DebugFrameListener g_DebugFrameListener(g_EventBus);

    //subscribing to the key pressed event
    g_EventBus.Subscribe<Engine::KeyPressedEvent>([](const Engine::KeyPressedEvent& e) {
            if (e.key == GLFW_KEY_ESCAPE) 
            {
                printf("ESCAPE\n");
            }
        });

    // ===== ECS CAMERA ENTITY =====
    Engine::ECS::EntityId cameraEntity = g_World.CreateEntity();
    auto& cameraTransform = g_World.AddComponent <Engine::ECS::Transform>(cameraEntity);
    cameraTransform.position = { 0.0f,0.0f,0.0f };
    cameraTransform.scale = { 1.0f,1.0f,1.0f };
    cameraTransform.rotationZ = 0.0f;

    // Camera component
    auto& camera2D = g_World.AddComponent<Engine::ECS::Camera2D>(cameraEntity);
    camera2D.zoom = 1.0f;   // see 1 unit vertically (like your current setup)
    camera2D.nearClip = -1.0f;
    camera2D.farClip = 1.0f;
    camera2D.primary = true;

    // ===== ECS CREATE ENTITY =====
    Engine::ECS::EntityId quadEntity = g_World.CreateEntity();
    auto& transform = g_World.AddComponent<Engine::ECS::Transform>(quadEntity);
    transform.position = { 0.0f, 0.0f, 0.0f };
    transform.scale = { 1.0f, 1.0f, 1.0f };
    transform.rotationZ = 0.0f;

    // add sprite component (what gets drawn)
    auto& sprite = g_World.AddComponent<Engine::ECS::Sprite2D>(quadEntity);
    sprite.size = { 0.2f, 0.2f };                 // visible size in NDC-ish world
    sprite.color = { 1.0f, 0.5f, 0.2f, 1.0f };     // orange-ish

    //create render system
    Engine::RenderSystem renderSystem(g_World, g_EventBus);

    // engine loop
    while (!glfwWindowShouldClose(window))
    {
        double now = glfwGetTime();

        //begin system frame
        Engine::TimeSystem::BeginFrame(g_Time, now);
        Engine::InputSystem::BeginFrame();
        glfwPollEvents(); // keyboard/mouse/window events

        g_EventBus.Emit(Engine::FrameStartEvent{ g_Time });

        //===== FIXED UPDATE =====
        int stepIndex = 0;
        while (Engine::TimeSystem::StepFixed(g_Time))
        {
            g_EventBus.Emit(Engine::FixedUpdateEvent
                {
                    g_Time,
                    g_Time.fixedDeltaTime,
                    stepIndex++
                });

            transform.rotationZ += g_Time.fixedDeltaTime;
            cameraTransform.position.x = std::sin(g_Time.timeSinceStart) * 0.5f;
            // PhysicsSystem::FixedUpdate(g_Time.fixedDeltaTime);
        }

        //===== UPDATE =====
        g_EventBus.Emit(Engine::UpdateEvent{ g_Time });

        //manually checking input
        if (Engine::InputSystem::WasKeyPressed(GLFW_KEY_SPACE)) 
        { 
            printf("IM PRESSING SPACE\n");
        }

        //===== RENDER =====
        float alpha = 0.0f;
        if (g_Time.fixedDeltaTime > 0.0f)
        {
            alpha = std::clamp(
                g_Time.accumulator / g_Time.fixedDeltaTime,
                0.0f,
                1.0f
            );
        }

        // clears the buffer and sets it to this colour
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        g_EventBus.Emit(Engine::RenderEvent{ g_Time,alpha });
        
        //===== FRAME END =====
        g_EventBus.Emit(Engine::FrameEndEvent{ g_Time });

        glfwSwapBuffers(window); // show rendered frame
    }

    //cleanup and terminate window
    glfwTerminate();
    return 0;
}