#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "scene_load_error.hpp"
#include "serialization/lscene_value.hpp"

namespace Ludus::SceneValues
{
	using Object = Serialization::LSceneValue::Object;

	void AddError(
		std::vector<SceneLoadError>& errors,
		const Serialization::LSceneValue& value,
		std::string message);

	const Object* RequireObject(
		const Serialization::LSceneValue& value,
		std::string_view message,
		std::vector<SceneLoadError>& errors);

	const Serialization::LSceneValue* FindField(
		const Object& fields,
		std::string_view name);

	bool ValidateFields(
		const Object& fields,
		std::initializer_list<std::string_view> allowed,
		std::string_view owner,
		std::vector<SceneLoadError>& errors);

	const std::string* RequiredString(
		const Object& fields,
		std::string_view name,
		const Serialization::LSceneValue& owner,
		std::vector<SceneLoadError>& errors);

	bool OptionalString(
		const Object& fields,
		std::string_view name,
		std::string& output,
		std::vector<SceneLoadError>& errors);

	bool OptionalBoolean(
		const Object& fields,
		std::string_view name,
		bool& output,
		std::vector<SceneLoadError>& errors);

	bool FiniteFloat(
		const Serialization::LSceneValue& value,
		float& output);

	bool OptionalVec3(
		const Object& fields,
		std::string_view name,
		glm::vec3 defaultValue,
		glm::vec3& output,
		std::vector<SceneLoadError>& errors);

	Serialization::LSceneValue WriteVec3(glm::vec3 value);
}
