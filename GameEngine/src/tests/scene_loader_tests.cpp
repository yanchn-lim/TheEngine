#include "scene_loader_tests.hpp"

#include <algorithm>
#include <string_view>

#include "assets/asset_manager.hpp"
#include "components/renderable.hpp"
#include "components/transform.hpp"
#include "rotator.hpp"
#include "scene/scene.hpp"
#include "scene/scene_component_registry.hpp"
#include "scene/scene_loader.hpp"

namespace Tests
{
	namespace
	{
		bool LoadOrderedComponents(
			const char* path,
			Components::Transform& transform,
			Rotator& rotator)
		{
			Assets::AssetManager assets;
			Ludus::Scene scene;
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			RegisterRotatorSceneComponent(components);

			std::vector<Ludus::SceneLoadError> errors;
			if (!Ludus::SceneLoader::Load(path, scene, assets, components, errors))
				return false;

			const ECS::Entity entity = scene.FindEntity("test_entity");
			const Components::Transform* loadedTransform =
				scene.GetWorld().TryGetComponent<Components::Transform>(entity);
			const Rotator* loadedRotator = scene.GetWorld().TryGetComponent<Rotator>(entity);
			if (!loadedTransform || !loadedRotator)
				return false;

			transform = *loadedTransform;
			rotator = *loadedRotator;
			return true;
		}

		bool ComponentOrderIsIndependent()
		{
			Components::Transform firstTransform;
			Components::Transform secondTransform;
			Rotator firstRotator;
			Rotator secondRotator;
			if (!LoadOrderedComponents(
					"assets/scenes/tests/component_order_a.lscene",
					firstTransform,
					firstRotator) ||
				!LoadOrderedComponents(
					"assets/scenes/tests/component_order_b.lscene",
					secondTransform,
					secondRotator))
			{
				return false;
			}

			return
				firstTransform.position.x == secondTransform.position.x &&
				firstTransform.position.y == secondTransform.position.y &&
				firstTransform.position.z == secondTransform.position.z &&
				firstTransform.scale.x == secondTransform.scale.x &&
				firstTransform.scale.y == secondTransform.scale.y &&
				firstTransform.scale.z == secondTransform.scale.z &&
				firstTransform.rotation.w == secondTransform.rotation.w &&
				firstTransform.rotation.x == secondTransform.rotation.x &&
				firstTransform.rotation.y == secondTransform.rotation.y &&
				firstTransform.rotation.z == secondTransform.rotation.z &&
				firstRotator.axis.x == secondRotator.axis.x &&
				firstRotator.axis.y == secondRotator.axis.y &&
				firstRotator.axis.z == secondRotator.axis.z &&
				firstRotator.radiansPerSecond == secondRotator.radiansPerSecond;
		}

		bool HasLoadError(
			const char* path,
			std::string_view expectedMessage,
			bool makeSceneNonEmpty = false,
			bool requireEmptyAfterFailure = false)
		{
			Assets::AssetManager assets;
			Ludus::Scene scene;
			Ludus::SceneComponentRegistry components;
			Ludus::RegisterBuiltInSceneComponents(components);
			RegisterRotatorSceneComponent(components);

			if (makeSceneNonEmpty)
				scene.CreateEntity("existing", "Existing");

			std::vector<Ludus::SceneLoadError> errors;
			if (Ludus::SceneLoader::Load(path, scene, assets, components, errors))
				return false;

			const bool hasError = std::ranges::any_of(errors,
				[&](const Ludus::SceneLoadError& error)
				{
					return error.path == path && error.message == expectedMessage;
				});
			return hasError &&
				(!requireEmptyAfterFailure || scene.GetWorld().GetEntityCount() == 0);
		}
	}

	bool RunSceneLoaderTests()
	{
		Assets::AssetManager assets;
		Ludus::Scene scene;
		Ludus::SceneComponentRegistry components;
		Ludus::RegisterBuiltInSceneComponents(components);
		RegisterRotatorSceneComponent(components);

		std::vector<Ludus::SceneLoadError> errors;
		if (!Ludus::SceneLoader::Load(
			"assets/scenes/maxwell.lscene",
			scene,
			assets,
			components,
			errors))
		{
			return false;
		}

		const ECS::Entity left = scene.FindEntity("maxwell_left");
		const ECS::Entity right = scene.FindEntity("maxwell_right");
		const bool loadedSceneValid = errors.empty() &&
			left.IsValid() &&
			right.IsValid() &&
			scene.GetEntityName("maxwell_left") == "Maxwell Left" &&
			scene.GetWorld().HasComponent<Components::Transform>(left) &&
			scene.GetWorld().HasComponent<Components::Renderable>(left) &&
			scene.GetWorld().HasComponent<Rotator>(left) &&
			 scene.GetWorld().HasComponent<Components::Transform>(right) &&
			 scene.GetWorld().HasComponent<Components::Renderable>(right) &&
			 scene.GetWorld().HasComponent<Rotator>(right);

		if (!loadedSceneValid)
			return false;

		scene.GetWorld().RemoveEntity(left);
		if (scene.FindEntity("maxwell_left").IsValid() ||
			!scene.GetEntityName("maxwell_left").empty())
			return false;

		const ECS::Entity replacement = scene.CreateEntity("maxwell_left", "Replacement");
		if (!replacement.IsValid() || !scene.RemoveEntity("maxwell_left") ||
			scene.GetWorld().IsEntityAlive(replacement) ||
			scene.FindEntity("maxwell_left").IsValid() ||
			!scene.GetEntityName("maxwell_left").empty())
			return false;

		return ComponentOrderIsIndependent() &&
			 HasLoadError(
				 "assets/scenes/tests/unknown_component.lscene",
				 "unknown component 'MissingComponent'") &&
			 HasLoadError(
				 "assets/scenes/tests/unknown_asset.lscene",
				 "unknown mesh 'missing_mesh'") &&
			 HasLoadError(
				 "assets/scenes/tests/duplicate_entity.lscene",
				 "duplicate key 'same_entity'") &&
			 HasLoadError(
				 "assets/scenes/tests/invalid_component_property.lscene",
				 "unknown component field 'wrong_field'") &&
			 HasLoadError(
				 "assets/scenes/tests/valid_empty.lscene",
				 "scene must be empty before loading",
				 true) &&
			 HasLoadError(
				 "assets/scenes/tests/float_overflow.lscene",
				 "field 'position' requires three numbers") &&
			 HasLoadError(
				 "assets/scenes/tests/zero_rotator.lscene",
				 "Rotator.axis must not be zero") &&
			 HasLoadError(
				 "assets/scenes/tests/missing_material.lscene",
				 "Renderable requires material_override because mesh 'maxwell' has surfaces without materials",
				 false,
				 true) &&
			 HasLoadError(
				 "assets/scenes/tests/partial_entity.lscene",
				 "unknown component 'ZInvalid'",
				 false,
				 true);
	}
}
