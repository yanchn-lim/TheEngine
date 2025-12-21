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
            auto& world = ctx.world;

            // Camera
            m_CameraEntity = world.CreateEntity();

            auto& camTransform = world.AddComponent<ECS::Transform>(m_CameraEntity);
            camTransform.position = { 0.0f, 0.0f, 0.0f };
            camTransform.scale = { 1.0f, 1.0f, 1.0f };
            camTransform.rotationZ = 0.0f;

            auto& camera = world.AddComponent<ECS::Camera2D>(m_CameraEntity);
            camera.zoom = 1.0f;
            camera.nearClip = -1.0f;
            camera.farClip = 1.0f;
            camera.primary = true;

            // Quad
            m_QuadEntity = world.CreateEntity();

            auto& quadTransform = world.AddComponent<ECS::Transform>(m_QuadEntity);
            quadTransform.position = { 0.0f, 0.0f, 0.0f };
            quadTransform.scale = { 1.0f, 1.0f, 1.0f };
            quadTransform.rotationZ = 0.0f;

            auto& quadSprite = world.AddComponent<ECS::Sprite2D>(m_QuadEntity);
            quadSprite.size = { 0.2f, 0.2f };
            quadSprite.color = { 1.0f, 0.5f, 0.2f, 1.0f, };
        }

        void OnFixedUpdate(float fixedDeltaTime) override
        {
            auto& ctx = GetContext();
            auto& world = ctx.world;

            auto* transform = world.GetComponent<ECS::Transform>(m_QuadEntity);
            if (!transform) return;

            transform->rotationZ += fixedDeltaTime; // spin
        }

        void OnUpdate(float /*deltaTime*/) override
        {
            // per-frame (non-physics) logic can go here
        }

    private:
        ECS::EntityId m_CameraEntity = ECS::kInvalidEntity;
        ECS::EntityId m_QuadEntity = ECS::kInvalidEntity;
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
