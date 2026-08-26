#include "rotator_system.hpp"

#include <cmath>

#include <glm/gtc/quaternion.hpp>

#include "components/transform.hpp"
#include "ecs/ecs_world.hpp"
#include "rotator.hpp"
#include "scene/scene_component_registry.hpp"
#include "scene/scene_value_reader.hpp"

namespace Ludus
{
	template<>
	struct SceneComponentCodec<Tests::Rotator>
	{
		static constexpr std::string_view Name = "Rotator";

		static bool Load(
			const Serialization::LSceneValue& value,
			const SceneAssetContext&,
			Tests::Rotator& rotator,
			std::vector<SceneLoadError>& errors)
		{
			const auto* fields = SceneValues::RequireObject(
				value, "Rotator must be a block", errors);
			if (!fields || !SceneValues::ValidateFields(
				*fields, { "axis", "speed_degrees" }, "Rotator", errors))
				return false;

			glm::vec3 axis;
			if (!SceneValues::OptionalVec3(
				*fields, "axis", { 0.0f, 1.0f, 0.0f }, axis, errors))
				return false;

			if (glm::dot(axis, axis) <= 1.0e-12f)
			{
				const auto* axisField = SceneValues::FindField(*fields, "axis");
				SceneValues::AddError(
					errors, axisField ? *axisField : value, "Rotator.axis must not be zero");
				return false;
			}
			rotator.axis = glm::normalize(axis);

			const auto speedField = fields->find("speed_degrees");
			if (speedField == fields->end())
			{
				SceneValues::AddError(errors, value, "Rotator requires speed_degrees");
				return false;
			}
			float speed = 0.0f;
			if (!SceneValues::FiniteFloat(speedField->second, speed))
			{
				SceneValues::AddError(
					errors, speedField->second, "Rotator.speed_degrees must be a number");
				return false;
			}
			rotator.radiansPerSecond = glm::radians(speed);
			return true;
		}

		static bool Save(
			const Tests::Rotator& rotator,
			const SceneAssetContext&,
			Serialization::LSceneValue& output,
			std::vector<std::string>& errors)
		{
			const bool finiteAxis =
				std::isfinite(rotator.axis.x) &&
				std::isfinite(rotator.axis.y) &&
				std::isfinite(rotator.axis.z);
			if (!finiteAxis || glm::dot(rotator.axis, rotator.axis) <= 1.0e-12f ||
				!std::isfinite(rotator.radiansPerSecond))
			{
				errors.push_back("Rotator contains an invalid numeric value");
				return false;
			}

			Serialization::LSceneValue::Object fields;
			fields.emplace("axis", SceneValues::WriteVec3(glm::normalize(rotator.axis)));
			fields.emplace("speed_degrees", Serialization::LSceneValue::Float(
				glm::degrees(rotator.radiansPerSecond), {}));
			output = Serialization::LSceneValue::ObjectValue(std::move(fields));
			return true;
		}
	};
}

namespace Tests
{
	void RotatorSystem::OnFixedUpdate(ECS::World& world, double fixedDeltaTime)
	{
		world.ForEach<Components::Transform, Rotator>(
			[fixedDeltaTime](ECS::Entity, Components::Transform& transform, const Rotator& rotator)
			{
				const float angle = rotator.radiansPerSecond * static_cast<float>(fixedDeltaTime);
				transform.rotation = glm::normalize(
					glm::angleAxis(angle, rotator.axis) * transform.rotation);
			});
	}

	void RegisterRotatorSceneComponent(Ludus::SceneComponentRegistry& registry)
	{
		registry.Register<Rotator>();
	}
}
