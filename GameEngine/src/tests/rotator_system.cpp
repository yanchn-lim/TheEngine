#include "rotator_system.hpp"

#include <glm/gtc/quaternion.hpp>

#include "components/transform.hpp"
#include "ecs/ecs_world.hpp"
#include "rotator.hpp"
#include "scene/scene_component_registry.hpp"
#include "scene/scene_value_reader.hpp"

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
		registry.Register("Rotator",
			[](const Serialization::LSceneValue& value,
				const Ludus::SceneAssetContext&,
				ECS::World& world,
				ECS::Entity entity,
				std::vector<Ludus::SceneLoadError>& errors)
			{
				const auto* fields = Ludus::SceneValues::RequireObject(
					value, "Rotator must be a block", errors);
				if (!fields || !Ludus::SceneValues::ValidateFields(
					*fields, { "axis", "speed_degrees" }, "Rotator", errors))
					return false;

				Rotator rotator;
				glm::vec3 axis;
				if (!Ludus::SceneValues::OptionalVec3(
					*fields, "axis", { 0.0f, 1.0f, 0.0f }, axis, errors))
					return false;

				if (glm::dot(axis, axis) <= 1.0e-12f)
				{
					const auto* axisField = Ludus::SceneValues::FindField(*fields, "axis");
					Ludus::SceneValues::AddError(
						errors, axisField ? *axisField : value, "Rotator.axis must not be zero");
					return false;
				}
				rotator.axis = glm::normalize(axis);

				const auto speedField = fields->find("speed_degrees");
				if (speedField == fields->end())
				{
					Ludus::SceneValues::AddError(errors, value, "Rotator requires speed_degrees");
					return false;
				}
				float speed = 0.0f;
				if (!Ludus::SceneValues::FiniteFloat(speedField->second, speed))
				{
					Ludus::SceneValues::AddError(
						errors, speedField->second, "Rotator.speed_degrees must be a number");
					return false;
				}
				rotator.radiansPerSecond = glm::radians(speed);

				world.AddComponent(entity, rotator);
				return true;
			});
	}
}
