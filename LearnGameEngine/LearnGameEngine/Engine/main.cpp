#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

#include "Core/time.hpp"
#include "Core/engine_context.hpp"
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

Engine::EngineCtx g_Context;

static void GLFWErrorCallback(int error, const char* description)
{
    std::cerr << "GLFW Error (" << error << "): " << description << "\n";
}

// --- Window resize callback -------------------------------------------------
static void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    (void)window;
    glViewport(0, 0, width, height);
    g_Context.windowWidth = width;
    g_Context.windowHeight = height;
}

void MainLoop(GLFWwindow* window)
{
   
}

int main()
{
    //init glfw
    glfwSetErrorCallback(GLFWErrorCallback);

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // telling glfw that i want v4
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5); // specifically v4.5
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // forces VAOs,shaders,etc
#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    g_Context.windowWidth = 1600;
    g_Context.windowHeight = 900;

    g_Context.window = glfwCreateWindow(
        g_Context.windowWidth,
        g_Context.windowHeight,
        "Engine Sandbox",
        nullptr,
        nullptr
    );

    if (!g_Context.window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(g_Context.window);
    glfwSetFramebufferSizeCallback(g_Context.window, FramebufferSizeCallback);

    //vsync
    glfwSwapInterval(1);

    //init GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(g_Context.window);
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, g_Context.windowWidth, g_Context.windowHeight);

    //pretty much singletons here
    //initializing global systems
    double startTime = glfwGetTime();
    Engine::TimeSystem::Initialize(g_Context.time, startTime, g_Context.timeConfig);
    Engine::EventBus& eventBus = g_Context.eventBus;
    Engine::InputSystem::Initialize(g_Context.window, &eventBus);

    // plug in debug frame listener
    Engine::DebugFrameListener g_DebugFrameListener(g_Context.eventBus);

    //subscribing to the key pressed event
    g_Context.eventBus.Subscribe<Engine::KeyPressedEvent>([](const Engine::KeyPressedEvent& e) {
            if (e.key == GLFW_KEY_ESCAPE) 
            {
                printf("ESCAPE\n");
            }
        });

    // ===== ECS CAMERA ENTITY =====
    Engine::ECS::EntityId cameraEntity = g_Context.world.CreateEntity();
    auto& cameraTransform = g_Context.world.AddComponent <Engine::ECS::Transform>(cameraEntity);
    cameraTransform.position = { 0.0f,0.0f,0.0f };
    cameraTransform.scale = { 1.0f,1.0f,1.0f };
    cameraTransform.rotationZ = 0.0f;

    // Camera component
    auto& camera2D = g_Context.world.AddComponent<Engine::ECS::Camera2D>(cameraEntity);
    camera2D.zoom = 1.0f;   // see 1 unit vertically (like your current setup)
    camera2D.nearClip = -1.0f;
    camera2D.farClip = 1.0f;
    camera2D.primary = true;

    // ===== ECS CREATE ENTITY =====
    Engine::ECS::EntityId quadEntity = g_Context.world.CreateEntity();
    auto& transform = g_Context.world.AddComponent<Engine::ECS::Transform>(quadEntity);
    transform.position = { 0.0f, 0.0f, 0.0f };
    transform.scale = { 1.0f, 1.0f, 1.0f };
    transform.rotationZ = 0.0f;

    // add sprite component (what gets drawn)
    auto& sprite = g_Context.world.AddComponent<Engine::ECS::Sprite2D>(quadEntity);
    sprite.size = { 0.2f, 0.2f };                 // visible size in NDC-ish world
    sprite.color = { 1.0f, 0.5f, 0.2f, 1.0f };     // orange-ish

    //create render system
    Engine::RenderSystem renderSystem(g_Context.world, g_Context.eventBus);

    eventBus.Subscribe<Engine::FixedUpdateEvent>(
        [&](const Engine::FixedUpdateEvent& e)
        {
            auto* t = g_Context.world.GetComponent<Engine::ECS::Transform>(quadEntity);
            if (!t) return;
            t->rotationZ += e.fixedDeltaTime; // spin at 1 rad/sec
        }
    );


    // engine loop
    while (!glfwWindowShouldClose(g_Context.window))
    {
        double now = glfwGetTime();

        //begin system frame
        Engine::TimeSystem::BeginFrame(g_Context.time, now);
        Engine::InputSystem::BeginFrame();
        glfwPollEvents(); // keyboard/mouse/window events

        g_Context.eventBus.Emit(Engine::FrameStartEvent{ g_Context.time });

        //===== FIXED UPDATE =====
        int stepIndex = 0;
        while (Engine::TimeSystem::StepFixed(g_Context.time))
        {
            Engine::FixedUpdateEvent fixedEvent{
                g_Context.time,
                g_Context.time.fixedDeltaTime,
                stepIndex++
            };

            eventBus.Emit(fixedEvent);;
            // PhysicsSystem::FixedUpdate(g_Time.fixedDeltaTime);
        }

        //===== UPDATE =====
        g_Context.eventBus.Emit(Engine::UpdateEvent{ g_Context.time });

        //manually checking input
        if (Engine::InputSystem::WasKeyPressed(GLFW_KEY_SPACE)) 
        { 
            printf("IM PRESSING SPACE\n");
        }

        //===== RENDER =====
        float alpha = 0.0f;
        if (g_Context.time.fixedDeltaTime > 0.0f)
        {
            alpha = std::clamp(
                g_Context.time.accumulator / g_Context.time.fixedDeltaTime,
                0.0f,
                1.0f
            );
        }

        // clears the buffer and sets it to this colour
        glClearColor(0.0f, 0.4f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        eventBus.Emit(Engine::RenderEvent{ g_Context.time,alpha });
        
        //===== FRAME END =====
        eventBus.Emit(Engine::FrameEndEvent{ g_Context.time });

        glfwSwapBuffers(g_Context.window); // show rendered frame
    }

    //cleanup and terminate window
    glfwDestroyWindow(g_Context.window);
    glfwTerminate();
    return 0;
}