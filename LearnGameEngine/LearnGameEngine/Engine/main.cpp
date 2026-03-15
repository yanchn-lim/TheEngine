#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <string>


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
#include "ECS/ecs_types.hpp"
#include "ECS/components.hpp"
#include "ECS/entity.hpp"

#include "imgui.h"

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
            Engine::Entity camera(world.CreateEntity(), &world,"Camera");

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
            Engine::Entity quad(world.CreateEntity(), &world,"Quad");

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

        void OnRenderGUI() override
        {
            //DrawDockspace();          // optional, later
            DrawSceneHierarchyPanel();
            DrawInspectorPanel();
        }

        void DrawSceneHierarchyPanel()
        {
            auto& ctx = GetContext();
            auto& scene = ctx.sceneManager.GetActiveScene();
            auto& world = scene.GetWorld();

            ImGui::Begin("Scene Hierarchy");

            // naive: iterate over all entities in world (depends on your ECS API)
            world.ForEach<Engine::ECS::Transform>(
                [&](Engine::ECS::EntityId id, Engine::ECS::Transform&)
                {
                    std::string label = "Entity " + std::to_string(id);

                    bool selected = (id == m_SelectedEntityId);
                    if (ImGui::Selectable(label.c_str(), selected))
                    {
                        m_SelectedEntityId = id;
                    }
                }
            );

            ImGui::End();
        }

        void DrawInspectorPanel()
        {
            if (m_SelectedEntityId == Engine::ECS::kInvalidEntity)
                return;

            auto& ctx = GetContext();
            auto& scene = ctx.sceneManager.GetActiveScene();
            auto& world = scene.GetWorld();

            ImGui::Begin("Inspector");

            if (auto* t = world.GetComponent<Engine::ECS::Transform>(m_SelectedEntityId))
            {
                ImGui::Text("Transform");
                ImGui::DragFloat3("Position", &t->position.x, 0.1f);
                ImGui::DragFloat("RotationZ", &t->rotationZ, 0.01f);
                ImGui::DragFloat3("Scale", &t->scale.x, 0.1f);
            }

            if (auto* sprite = world.GetComponent<Engine::ECS::Sprite2D>(m_SelectedEntityId))
            {
                ImGui::Separator();
                ImGui::Text("Sprite2D");
                ImGui::DragFloat2("Size", &sprite->size.x, 0.01f);
                ImGui::ColorEdit4("Color", &sprite->color.x);
            }

            ImGui::End();
        }

    private:
        ECS::EntityId quadId;
        ECS::EntityId camId;
        ECS::EntityId m_SelectedEntityId = Engine::ECS::kInvalidEntity;
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
