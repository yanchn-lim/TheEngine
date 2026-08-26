#include "scene_component_registry.hpp"

#include <glm/gtc/quaternion.hpp>

#include "components/renderable.hpp"
#include "components/transform.hpp"
#include "ecs/ecs_world.hpp"
#include "scene_value_reader.hpp"

namespace Ludus
{
	namespace
	{
		bool LoadTransform(
			const Serialization::LSceneValue& value,
			const SceneAssetContext&,
			ECS::World& world,
			ECS::Entity entity,
			std::vector<SceneLoadError>& errors)
		{
			const SceneValues::Object* fields = SceneValues::RequireObject(
				value, "component must be a block", errors);
			if (!fields) return false;

			if (!SceneValues::ValidateFields(
				*fields, { "position", "rotation_degrees", "scale" }, "component", errors))
				return false;

			Components::Transform transform;
			glm::vec3 rotationDegrees{};
			if (!SceneValues::OptionalVec3(*fields, "position", {}, transform.position, errors) ||
				!SceneValues::OptionalVec3(*fields, "rotation_degrees", {}, rotationDegrees, errors) ||
				!SceneValues::OptionalVec3(*fields, "scale", { 1.0f, 1.0f, 1.0f }, transform.scale, errors))
				return false;

			transform.rotation = glm::quat(glm::radians(rotationDegrees));
			world.AddComponent(entity, transform);
			return true;
		}

		bool LoadRenderable(
			const Serialization::LSceneValue& value,
			const SceneAssetContext& assets,
			ECS::World& world,
			ECS::Entity entity,
			std::vector<SceneLoadError>& errors)
		{
			const SceneValues::Object* fields = SceneValues::RequireObject(
				value, "component must be a block", errors);
			if (!fields) return false;
			if (!SceneValues::ValidateFields(
				*fields, { "mesh", "material_override", "visible" }, "component", errors))
				return false;

			const auto meshField = fields->find("mesh");
			if (meshField == fields->end() || !meshField->second.TryGetString())
			{
				SceneValues::AddError(errors, value, "Renderable requires string field 'mesh'");
				return false;
			}
			const std::string& meshName = *meshField->second.TryGetString();
			const auto mesh = assets.meshes.find(meshName);
			if (mesh == assets.meshes.end())
			{
				SceneValues::AddError(errors, meshField->second, "unknown mesh '" + meshName + "'");
				return false;
			}

			Components::Renderable renderable;
			renderable.mesh = mesh->second;
			if (const auto materialField = fields->find("material_override"); materialField != fields->end())
			{
				const std::string* name = materialField->second.TryGetString();
				if (!name)
				{
					SceneValues::AddError(errors, materialField->second, "field 'material_override' must be a string");
					return false;
				}
				const auto material = assets.materials.find(*name);
				if (material == assets.materials.end())
				{
					SceneValues::AddError(errors, materialField->second, "unknown material '" + *name + "'");
					return false;
				}
				renderable.materialOverride = material->second;
			}

			const auto defaults = assets.meshHasDefaultMaterials.find(meshName);
			const bool hasDefaultMaterials =
				defaults != assets.meshHasDefaultMaterials.end() && defaults->second;
			if (!renderable.materialOverride && !hasDefaultMaterials)
			{
				SceneValues::AddError(
					errors,
					meshField->second,
					"Renderable requires material_override because mesh '" +
						meshName + "' has surfaces without materials");
				return false;
			}

			if (!SceneValues::OptionalBoolean(*fields, "visible", renderable.visible, errors))
				return false;
			world.AddComponent(entity, renderable);
			return true;
		}
	}

	bool SceneComponentRegistry::Register(std::string name, Loader loader)
	{
		return !name.empty() && loader && _loaders.emplace(std::move(name), std::move(loader)).second;
	}

	bool SceneComponentRegistry::Load(
		std::string_view name,
		const Serialization::LSceneValue& value,
		const SceneAssetContext& assets,
		ECS::World& world,
		ECS::Entity entity,
		std::vector<SceneLoadError>& errors) const
	{
		const auto found = _loaders.find(std::string(name));
		if (found == _loaders.end())
		{
			SceneValues::AddError(errors, value, "unknown component '" + std::string(name) + "'");
			return false;
		}
		return found->second(value, assets, world, entity, errors);
	}

	void RegisterBuiltInSceneComponents(SceneComponentRegistry& registry)
	{
		registry.Register("Transform", LoadTransform);
		registry.Register("Renderable", LoadRenderable);
	}
}
