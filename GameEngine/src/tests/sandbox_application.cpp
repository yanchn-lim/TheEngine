#include "sandbox_application.hpp"

#include <vector>

#include "core/engine.hpp"
#include "debug/debug.hpp"
#include "rotator.hpp"
#include "rotator_system.hpp"
#include "scene/scene_component_registry.hpp"
#include "scene/scene_loader.hpp"
#include "systems/render_system.hpp"

namespace Tests
{
    bool SandboxApplication::OnInitialize(Ludus::Engine& engine)
    {
        if (!LoadScene(engine, "assets/scenes/maxwell.lscene"))
            return false;

        ECS::World& world = _scene.GetWorld();
        world.AddSystem<RotatorSystem>();
        world.AddSystem<Systems::RenderSystem>(engine.GetRenderEngine());

        return true;
    }

	bool SandboxApplication::LoadScene(Ludus::Engine& engine, const char* path)
	{
		Ludus::SceneComponentRegistry components;
		Ludus::RegisterBuiltInSceneComponents(components);
		RegisterRotatorSceneComponent(components);

		std::vector<Ludus::SceneLoadError> errors;
		if (Ludus::SceneLoader::Load(path, _scene, engine.GetAssets(), components, errors))
			return true;

		for (const Ludus::SceneLoadError& error : errors)
		{
			Debug::LogError(
				error.path, ":",
				error.location.line, ":",
				error.location.column, ": ",
				error.message);
		}
		return false;
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
