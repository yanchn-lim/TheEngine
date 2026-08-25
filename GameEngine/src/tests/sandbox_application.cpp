#include "sandbox_application.hpp"

#include <algorithm>
#include <vector>

#include "assets/asset_manager.hpp"
#include "assets/primitives/primitive_mesh2d.hpp"
#include "components/renderable.hpp"
#include "components/transform.hpp"
#include "core/engine.hpp"
#include "debug/debug.hpp"
#include "graphics/blend_mode.hpp"
#include "graphics_api_tests.hpp"
#include "movement_system.hpp"
#include "systems/render_system.hpp"

namespace Tests
{
    namespace
    {
        struct MovingQuad
        {
            glm::vec2 velocity;
        };

        class MovingQuadSystem final : public ECS::ISystem
        {
        public:
            static constexpr ECS::SystemPhase Phase = ECS::SystemPhase::UPDATE;
            static constexpr int Order = 100;

            void OnFixedUpdate(ECS::World& world, double fixedDeltaTime) override
            {
                world.ForEach<Components::Transform, MovingQuad>(
                    [fixedDeltaTime](
                        ECS::Entity,
                        Components::Transform& transform,
                        MovingQuad& movingQuad)
                    {
                        transform.position +=
                            glm::vec3(movingQuad.velocity, 0.0f) *
                            static_cast<float>(fixedDeltaTime);

                        constexpr float horizontalLimit = 1.65f;
                        constexpr float verticalLimit = 0.9f;

                        if (transform.position.x <= -horizontalLimit ||
                            transform.position.x >= horizontalLimit)
                        {
                            transform.position.x = std::clamp(
                                transform.position.x,
                                -horizontalLimit,
                                horizontalLimit);
                            movingQuad.velocity.x = -movingQuad.velocity.x;
                        }

                        if (transform.position.y <= -verticalLimit ||
                            transform.position.y >= verticalLimit)
                        {
                            transform.position.y = std::clamp(
                                transform.position.y,
                                -verticalLimit,
                                verticalLimit);
                            movingQuad.velocity.y = -movingQuad.velocity.y;
                        }
                    });
            }
        };

        bool EcsTestFailed(const char* message)
        {
            Debug::LogError("ECS self-test failed: ", message);
            return false;
        }

        struct SystemTrace
        {
            int createCount = 0;
            std::vector<int> updateOrder;
            std::vector<int> fixedUpdateOrder;
            double fixedDeltaTime = 0.0;
        };

        template<ECS::SystemPhase PhaseValue, int OrderValue, int Marker>
        class OrderedSystem final : public ECS::ISystem
        {
        public:
            static constexpr ECS::SystemPhase Phase = PhaseValue;
            static constexpr int Order = OrderValue;

            explicit OrderedSystem(SystemTrace& trace)
                : _trace(trace)
            {
            }

            void OnCreate(ECS::World&) override
            {
                ++_trace.createCount;
            }

            void OnUpdate(ECS::World&) override
            {
                _trace.updateOrder.push_back(Marker);
            }

            void OnFixedUpdate(ECS::World&, double fixedDeltaTime) override
            {
                _trace.fixedUpdateOrder.push_back(Marker);
                _trace.fixedDeltaTime = fixedDeltaTime;
            }

        private:
            SystemTrace& _trace;
        };

        bool RunEntityAndComponentTests()
        {
            ECS::World world;
            const ECS::Entity movingEntity = world.CreateEntity();
            const ECS::Entity positionOnlyEntity = world.CreateEntity();

            if (world.GetEntityCount() != 2 ||
                !world.IsEntityAlive(movingEntity) ||
                !world.IsEntityAlive(positionOnlyEntity))
            {
                return EcsTestFailed("entity creation or alive count is incorrect");
            }

            world.AddComponent(movingEntity, Position{ 10.0f, 20.0f });
            world.AddComponent(movingEntity, Velocity{ 2.0f, 3.0f });
            world.AddComponent(positionOnlyEntity, Position{ 40.0f, 50.0f });

            if (!world.HasComponent<Position>(movingEntity) ||
                !world.HasComponent<Velocity>(movingEntity) ||
                world.HasComponent<Velocity>(positionOnlyEntity))
            {
                return EcsTestFailed("component presence is incorrect");
            }

            if (!world.TryGetComponent<Position>(movingEntity) ||
                world.TryGetComponent<Velocity>(positionOnlyEntity))
            {
                return EcsTestFailed("component lookup is incorrect");
            }

            world.AddSystem<MovementSystem>();
            world.UpdateSystems();

            const Position& movedPosition = world.GetComponent<Position>(movingEntity);
            const Position& unchangedPosition = world.GetComponent<Position>(positionOnlyEntity);
            if (movedPosition.x != 12.0f || movedPosition.y != 23.0f ||
                unchangedPosition.x != 40.0f || unchangedPosition.y != 50.0f)
            {
                return EcsTestFailed("movement query updated the wrong component data");
            }

            std::size_t constVisitCount = 0;
            const ECS::World& constWorld = world;
            constWorld.ForEach<Position>(
                [&](ECS::Entity, const Position&)
                {
                    ++constVisitCount;
                });
            if (constVisitCount != 2)
                return EcsTestFailed("const query did not visit every matching entity");

            if (!world.RemoveComponent<Velocity>(movingEntity) ||
                world.RemoveComponent<Velocity>(movingEntity))
            {
                return EcsTestFailed("component removal result is incorrect");
            }

            world.UpdateSystems();
            const Position& positionAfterRemoval = world.GetComponent<Position>(movingEntity);
            if (positionAfterRemoval.x != 12.0f || positionAfterRemoval.y != 23.0f)
                return EcsTestFailed("system updated an entity after its required component was removed");

            world.RemoveEntity(movingEntity);
            if (world.IsEntityAlive(movingEntity) || world.GetEntityCount() != 1)
                return EcsTestFailed("entity removal is incorrect");

            const ECS::Entity reusedEntity = world.CreateEntity();
            if (reusedEntity.id != movingEntity.id ||
                reusedEntity.generation == movingEntity.generation ||
                world.IsEntityAlive(movingEntity))
            {
                return EcsTestFailed("entity slot reuse did not invalidate the old generation");
            }

            return true;
        }

        bool RunSystemOrderTests()
        {
            using PreUpdateSystem = OrderedSystem<ECS::SystemPhase::PREUPDATE, 500, 1>;
            using FirstUpdateSystem = OrderedSystem<ECS::SystemPhase::UPDATE, 100, 2>;
            using SecondUpdateSystem = OrderedSystem<ECS::SystemPhase::UPDATE, 200, 3>;
            using TiedUpdateSystem = OrderedSystem<ECS::SystemPhase::UPDATE, 200, 4>;

            ECS::World world;
            SystemTrace trace;
            world.AddSystem<SecondUpdateSystem>(trace);
            world.AddSystem<TiedUpdateSystem>(trace);
            world.AddSystem<PreUpdateSystem>(trace);
            world.AddSystem<FirstUpdateSystem>(trace);

            if (trace.createCount != 4)
                return EcsTestFailed("OnCreate did not run once for every system");

            world.UpdateSystems();
            if (trace.updateOrder != std::vector<int>{ 1, 2, 3, 4 })
                return EcsTestFailed("systems did not run by phase, order, and insertion order");

            constexpr double fixedDeltaTime = 1.0 / 60.0;
            world.FixedUpdateSystems(fixedDeltaTime);
            if (trace.fixedUpdateOrder != std::vector<int>{ 1, 2, 3, 4 } ||
                trace.fixedDeltaTime != fixedDeltaTime)
            {
                return EcsTestFailed("fixed systems did not receive the correct order or delta time");
            }

            return true;
        }

        bool RunEcsTests()
        {
            return RunEntityAndComponentTests() && RunSystemOrderTests();
        }
    }

    bool SandboxApplication::OnInitialize(Ludus::Engine& engine)
    {
#ifndef NDEBUG
        if (!RunGraphicsApiTests())
        {
            Debug::LogError("Graphics API self-tests failed");
            return false;
        }
        if (!RunEcsTests())
            return false;
#endif

        Assets::AssetManager& assets = engine.GetAssets();

        const Assets::ShaderHandle shader = assets.LoadShader(
            "assets/shaders/standard_gl.vert",
            "assets/shaders/standard_gl.frag",
            "assets/shaders/standard_vk.vert.spv",
            "assets/shaders/standard_vk.frag.spv");

        const Assets::TextureHandle texture =
            assets.LoadTexture("assets/textures/maxwell.png");

        if (!shader || !texture)
            return false;

        const Assets::MaterialHandle material = assets.CreateMaterial(
            "standard_material",
            shader,
            texture,
            { false, false, Graphics::BlendMode::NONE, true });

        const Assets::MeshHandle mesh = assets.CreateMesh(
            "builtin_quad",
            Assets::Primitive2D::Quad());

        if (!material || !mesh)
            return false;

        ECS::World& world = _scene.GetWorld();
        world.AddSystem<MovingQuadSystem>();
        world.AddSystem<Systems::RenderSystem>(engine.GetRenderEngine());

        constexpr int columns = 8;
        constexpr int rows = 5;

        for (int row = 0; row < rows; ++row)
        {
            for (int column = 0; column < columns; ++column)
            {
                const ECS::Entity entity = world.CreateEntity();

                Components::Transform transform;
                transform.position =
                {
                    -1.4f + static_cast<float>(column) * 0.4f,
                    -0.7f + static_cast<float>(row) * 0.35f,
                    0.0f
                };
                transform.scale = glm::vec3(0.14f);

                const float horizontalSpeed =
                    (row + column) % 2 == 0 ? 0.35f : -0.35f;
                const float verticalSpeed =
                    column % 2 == 0 ? 0.25f : -0.25f;

                world.AddComponent(entity, transform);
                world.AddComponent(
                    entity,
                    Components::Renderable{ mesh, material });
                world.AddComponent(
                    entity,
                    MovingQuad{ { horizontalSpeed, verticalSpeed } });
            }
        }

        return true;
    }

    void SandboxApplication::OnFixedUpdate(Ludus::Engine&, double fixedDeltaTime)
    {
        _scene.FixedUpdate(fixedDeltaTime);
    }

    void SandboxApplication::OnUpdate(Ludus::Engine&)
    {
        _scene.Update();
    }

    void SandboxApplication::OnImGui(Ludus::Engine&)
    {
        _debugOverlay.Draw();
    }

    void SandboxApplication::OnKey(Ludus::Engine&, int key, int action)
    {
        _debugOverlay.HandleKey(key, action);
    }

}
