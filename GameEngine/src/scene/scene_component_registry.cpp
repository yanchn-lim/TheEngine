#include "scene_component_registry.hpp"

#include <cmath>

#include <glm/gtc/quaternion.hpp>

#include "components/renderable.hpp"
#include "components/transform.hpp"
#include "scene_value_reader.hpp"

namespace Ludus
{
	namespace
	{
		template<typename Handle>
		size_t FindAssetName(
			const std::unordered_map<std::string, Handle>& assets,
			Handle handle,
			const std::string*& output)
		{
			output = nullptr;
			size_t matches = 0;
			for (const auto& [name, candidate] : assets)
				if (candidate.id == handle.id)
				{
					output = &name;
					++matches;
				}
			return matches;
		}

		bool IsFinite(glm::vec3 value)
		{
			return std::isfinite(value.x) &&
				std::isfinite(value.y) &&
				std::isfinite(value.z);
		}

		bool IsFinite(glm::quat value)
		{
			return std::isfinite(value.w) &&
				std::isfinite(value.x) &&
				std::isfinite(value.y) &&
				std::isfinite(value.z);
		}
	}

	template<>
	struct SceneComponentCodec<Components::Transform>
	{
		static constexpr std::string_view Name = "Transform";

		static bool Load(
			const Serialization::LSceneValue& value,
			const SceneAssetContext&,
			Components::Transform& transform,
			std::vector<SceneLoadError>& errors)
		{
			const SceneValues::Object* fields = SceneValues::RequireObject(
				value, "component must be a block", errors);
			if (!fields)
				return false;

			if (!SceneValues::ValidateFields(
				*fields, { "position", "rotation_degrees", "scale" }, "component", errors))
				return false;

			glm::vec3 rotationDegrees{};
			if (!SceneValues::OptionalVec3(*fields, "position", {}, transform.position, errors) ||
				!SceneValues::OptionalVec3(*fields, "rotation_degrees", {}, rotationDegrees, errors) ||
				!SceneValues::OptionalVec3(*fields, "scale", { 1.0f, 1.0f, 1.0f }, transform.scale, errors))
				return false;

			transform.rotation = glm::quat(glm::radians(rotationDegrees));
			return true;
		}

		static bool Save(
			const Components::Transform& transform,
			const SceneAssetContext&,
			Serialization::LSceneValue& output,
			std::vector<std::string>& errors)
		{
			if (!IsFinite(transform.position) || !IsFinite(transform.scale) ||
				!IsFinite(transform.rotation) ||
				glm::dot(transform.rotation, transform.rotation) <= 1.0e-12f)
			{
				errors.push_back("Transform contains an invalid numeric value");
				return false;
			}

			const glm::vec3 rotationDegrees = glm::degrees(
				glm::eulerAngles(glm::normalize(transform.rotation)));
			if (!IsFinite(rotationDegrees))
			{
				errors.push_back("Transform rotation cannot be converted to Euler degrees");
				return false;
			}

			Serialization::LSceneValue::Object fields;
			fields.emplace("position", SceneValues::WriteVec3(transform.position));
			fields.emplace("rotation_degrees", SceneValues::WriteVec3(rotationDegrees));
			fields.emplace("scale", SceneValues::WriteVec3(transform.scale));
			output = Serialization::LSceneValue::ObjectValue(std::move(fields));
			return true;
		}
	};

	template<>
	struct SceneComponentCodec<Components::Renderable>
	{
		static constexpr std::string_view Name = "Renderable";

		static bool Load(
			const Serialization::LSceneValue& value,
			const SceneAssetContext& assets,
			Components::Renderable& renderable,
			std::vector<SceneLoadError>& errors)
		{
			const SceneValues::Object* fields = SceneValues::RequireObject(
				value, "component must be a block", errors);
			if (!fields)
				return false;
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
			renderable.mesh = mesh->second;

			if (const auto materialField = fields->find("material_override"); materialField != fields->end())
			{
				const std::string* name = materialField->second.TryGetString();
				if (!name)
				{
					SceneValues::AddError(errors, materialField->second,
						"field 'material_override' must be a string");
					return false;
				}
				const auto material = assets.materials.find(*name);
				if (material == assets.materials.end())
				{
					SceneValues::AddError(errors, materialField->second,
						"unknown material '" + *name + "'");
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

			return SceneValues::OptionalBoolean(
				*fields, "visible", renderable.visible, errors);
		}

		static bool Save(
			const Components::Renderable& renderable,
			const SceneAssetContext& assets,
			Serialization::LSceneValue& output,
			std::vector<std::string>& errors)
		{
			const std::string* mesh = nullptr;
			const size_t meshMatches = FindAssetName(
				assets.meshes, renderable.mesh, mesh);
			if (meshMatches != 1)
			{
				errors.push_back(meshMatches == 0
					? "Renderable references a mesh outside the scene asset context"
					: "Renderable mesh handle has multiple scene aliases");
				return false;
			}

			Serialization::LSceneValue::Object fields;
			fields.emplace("mesh", Serialization::LSceneValue::String(*mesh, {}));
			const auto defaults = assets.meshHasDefaultMaterials.find(*mesh);
			const bool hasDefaultMaterials =
				defaults != assets.meshHasDefaultMaterials.end() && defaults->second;
			if (!renderable.materialOverride && !hasDefaultMaterials)
			{
				errors.push_back(
					"Renderable requires material_override because mesh '" +
					*mesh + "' has surfaces without materials");
				return false;
			}
			if (renderable.materialOverride)
			{
				const std::string* material = nullptr;
				const size_t materialMatches = FindAssetName(
					assets.materials, renderable.materialOverride, material);
				if (materialMatches != 1)
				{
					errors.push_back(materialMatches == 0
						? "Renderable references a material outside the scene asset context"
						: "Renderable material handle has multiple scene aliases");
					return false;
				}
				fields.emplace("material_override",
					Serialization::LSceneValue::String(*material, {}));
			}
			fields.emplace("visible",
				Serialization::LSceneValue::Boolean(renderable.visible, {}));
			output = Serialization::LSceneValue::ObjectValue(std::move(fields));
			return true;
		}
	};

	bool SceneComponentRegistry::RegisterEntry(
		std::string name,
		Loader loader,
		Saver saver,
		PresenceCheck has)
	{
		if (name.empty() || !loader || !saver || !has || _indices.contains(name))
			return false;

		const size_t index = _entries.size();
		_indices.emplace(name, index);
		_entries.push_back({ std::move(name), std::move(loader), std::move(saver), std::move(has) });
		return true;
	}

	bool SceneComponentRegistry::Load(
		std::string_view name,
		const Serialization::LSceneValue& value,
		const SceneAssetContext& assets,
		ECS::World& world,
		ECS::Entity entity,
		std::vector<SceneLoadError>& errors) const
	{
		const auto found = _indices.find(std::string(name));
		if (found == _indices.end())
		{
			SceneValues::AddError(errors, value, "unknown component '" + std::string(name) + "'");
			return false;
		}
		return _entries[found->second].load(value, assets, world, entity, errors);
	}

	bool SceneComponentRegistry::SaveComponents(
		const SceneAssetContext& assets,
		const ECS::World& world,
		ECS::Entity entity,
		Serialization::LSceneValue::Object& output,
		std::vector<std::string>& errors) const
	{
		size_t matchedComponents = 0;
		for (const Entry& entry : _entries)
		{
			if (!entry.has(world, entity))
				continue;
			++matchedComponents;

			Serialization::LSceneValue value = Serialization::LSceneValue::ObjectValue();
			if (!entry.save(assets, world, entity, value, errors))
				return false;
			output.emplace(entry.name, std::move(value));
		}

		if (matchedComponents != world.GetComponentCount(entity))
		{
			errors.push_back(
				"entity contains a component without a registered scene codec");
			return false;
		}
		return true;
	}

	void RegisterBuiltInSceneComponents(SceneComponentRegistry& registry)
	{
		registry.Register<Components::Transform>();
		registry.Register<Components::Renderable>();
	}
}
