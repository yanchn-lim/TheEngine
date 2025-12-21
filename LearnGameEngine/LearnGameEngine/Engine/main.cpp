#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>


/*=====GENERAL NAMING CONVENTIONS=====
    VARIABLES :
        - m_foo -> member variable (non-static)
        - g_foo -> global variable
        - s_foo -> static variable (member or function-local)
        - kFoo  -> constant value

    FUNCTION / Types  :
        - names in pamel case [GetThing() or PlayerController]
*/

// Engine/main.cpp
#include "Core/application.hpp"
#include "ECS/ecs.hpp"
#include "ECS/components.hpp"
#include "ECS/entity.hpp"

namespace
{
    using namespace Engine;

    class SandboxApp : public Application
    {
    public:
        using Application::Application;

    protected:
        void OnInitialize() override
        {
            auto& ctx = GetContext();
            auto& scene = ctx.sceneManager.GetActiveScene();
            auto& world = scene.GetWorld();

            // Camera
            Engine::Entity camera(world.CreateEntity(), &world);

            auto& camTransform = camera.AddComponent<ECS::Transform>();
            camTransform.position = { 0.0f, 0.0f, 0.0f };
            camTransform.scale = { 1.0f, 1.0f, 1.0f };
            camTransform.rotationZ = 0.0f;

            auto& camera2D = camera.AddComponent<ECS::Camera2D>();
            camera2D.zoom = 1.0f;
            camera2D.nearClip = -1.0f;
            camera2D.farClip = 1.0f;
            camera2D.primary = true;

            camId = camera.GetId();

            // Quad
            Engine::Entity quad(world.CreateEntity(), &world);

            auto& quadTransform = quad.AddComponent<Engine::ECS::Transform>();
            quadTransform.position = { 0.0f, 0.0f, 0.0f };
            quadTransform.scale = { 1.0f, 1.0f, 1.0f };
            quadTransform.rotationZ = 0.0f;

            auto& quadSprite = quad.AddComponent<Engine::ECS::Sprite2D>();
            quadSprite.size = { 0.2f, 0.2f };
            quadSprite.color = { 1.0f, 0.5f, 0.2f, 1.0f, };

            quadId = quad.GetId();
        }

        void OnFixedUpdate(float fixedDeltaTime) override
        {
            auto& ctx = GetContext();
            auto& scene = ctx.sceneManager.GetActiveScene();
            auto& world = scene.GetWorld();

            auto* transform = world.GetComponent<ECS::Transform>(quadId);
            if (!transform) return;

            transform->rotationZ += fixedDeltaTime; // spin
        }

        void OnUpdate(float /*deltaTime*/) override
        {
            // per-frame (non-physics) logic can go here
        }

    private:
        ECS::EntityId quadId;
        ECS::EntityId camId;
    };
}

int main()
{
    Engine::ApplicationConfig config;
    config.windowTitle = "Engine Sandbox";
    config.windowWidth = 1600;
    config.windowHeight = 900;

    SandboxApp app(config);
    return app.Run();
}
