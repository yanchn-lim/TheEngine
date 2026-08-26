#include "scene_value_reader.hpp"

#include <cmath>
#include <limits>

namespace Ludus::SceneValues
{
	void AddError(
		std::vector<SceneLoadError>& errors,
		const Serialization::LSceneValue& value,
		std::string message)
	{
		errors.push_back({ std::move(message), value.GetLocation() });
	}

	const Object* RequireObject(
		const Serialization::LSceneValue& value,
		std::string_view message,
		std::vector<SceneLoadError>& errors)
	{
		const Object* object = value.TryGetObject();
		if (!object)
			AddError(errors, value, std::string(message));
		return object;
	}

	const Serialization::LSceneValue* FindField(
		const Object& fields,
		std::string_view name)
	{
		const auto found = fields.find(std::string(name));
		return found == fields.end() ? nullptr : &found->second;
	}

	bool ValidateFields(
		const Object& fields,
		std::initializer_list<std::string_view> allowed,
		std::string_view owner,
		std::vector<SceneLoadError>& errors)
	{
		bool valid = true;
		for (const auto& [name, value] : fields)
		{
			bool known = false;
			for (const std::string_view allowedName : allowed)
				known = known || name == allowedName;
			if (known)
				continue;

			const std::string prefix = owner.empty()
				? "unknown field '"
				: "unknown " + std::string(owner) + " field '";
			AddError(errors, value, prefix + name + "'");
			valid = false;
		}
		return valid;
	}

	const std::string* RequiredString(
		const Object& fields,
		std::string_view name,
		const Serialization::LSceneValue& owner,
		std::vector<SceneLoadError>& errors)
	{
		const Serialization::LSceneValue* value = FindField(fields, name);
		if (!value)
		{
			AddError(errors, owner, "missing required field '" + std::string(name) + "'");
			return nullptr;
		}

		const std::string* text = value->TryGetString();
		if (!text)
			AddError(errors, *value, "field '" + std::string(name) + "' must be a string");
		return text;
	}

	bool OptionalString(
		const Object& fields,
		std::string_view name,
		std::string& output,
		std::vector<SceneLoadError>& errors)
	{
		const Serialization::LSceneValue* value = FindField(fields, name);
		if (!value)
		{
			output.clear();
			return true;
		}

		const std::string* text = value->TryGetString();
		if (!text)
		{
			AddError(errors, *value, "field '" + std::string(name) + "' must be a string");
			return false;
		}
		output = *text;
		return true;
	}

	bool OptionalBoolean(
		const Object& fields,
		std::string_view name,
		bool& output,
		std::vector<SceneLoadError>& errors)
	{
		const Serialization::LSceneValue* value = FindField(fields, name);
		if (!value)
			return true;

		const bool* boolean = value->TryGetBoolean();
		if (!boolean)
		{
			AddError(errors, *value, "field '" + std::string(name) + "' must be a boolean");
			return false;
		}
		output = *boolean;
		return true;
	}

	bool FiniteFloat(const Serialization::LSceneValue& value, float& output)
	{
		double number = 0.0;
		if (const double* floating = value.TryGetFloat()) number = *floating;
		else if (const int64_t* integer = value.TryGetInteger()) number = static_cast<double>(*integer);
		else return false;

		if (!std::isfinite(number) ||
			number < std::numeric_limits<float>::lowest() ||
			number > std::numeric_limits<float>::max())
		{
			return false;
		}
		output = static_cast<float>(number);
		return true;
	}

	bool OptionalVec3(
		const Object& fields,
		std::string_view name,
		glm::vec3 defaultValue,
		glm::vec3& output,
		std::vector<SceneLoadError>& errors)
	{
		const Serialization::LSceneValue* value = FindField(fields, name);
		if (!value)
		{
			output = defaultValue;
			return true;
		}

		const auto* array = value->TryGetArray();
		if (!array || array->size() != 3 ||
			!FiniteFloat((*array)[0], output.x) ||
			!FiniteFloat((*array)[1], output.y) ||
			!FiniteFloat((*array)[2], output.z))
		{
			AddError(errors, *value, "field '" + std::string(name) + "' requires three numbers");
			return false;
		}
		return true;
	}
}
