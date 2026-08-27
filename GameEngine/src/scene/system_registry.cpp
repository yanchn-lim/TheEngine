#include "system_registry.hpp"

#include <utility>

#include "scene_value_reader.hpp"

namespace Ludus
{
	namespace
	{
		bool IsIdentifier(std::string_view id)
		{
			if (id.empty() || (id.front() >= '0' && id.front() <= '9'))
				return false;
			for (const char character : id)
			{
				const bool valid =
					(character >= 'a' && character <= 'z') ||
					(character >= 'A' && character <= 'Z') ||
					(character >= '0' && character <= '9') ||
					character == '_' || character == '-';
				if (!valid)
					return false;
			}
			return true;
		}
	}

	bool SystemRegistry::Register(std::string id, Factory factory)
	{
		const std::string owner = "system '" + id + "' config";
		return Register(
			std::move(id),
			[owner](const Ludus::Serialization::LSceneValue& config,
				std::vector<SceneLoadError>& errors)
			{
				const auto* fields = SceneValues::RequireObject(
					config, owner + " must be a block", errors);
				return fields && SceneValues::ValidateFields(
					*fields, {}, owner, errors);
			},
			std::move(factory));
	}

	bool SystemRegistry::Register(
		std::string id,
		Validator validator,
		Factory factory)
	{
		if (!IsIdentifier(id) || !validator || !factory || _entries.contains(id))
			return false;

		_entries.emplace(
			std::move(id),
			Entry{ std::move(validator), std::move(factory) });
		return true;
	}

	bool SystemRegistry::Contains(std::string_view id) const
	{
		return _entries.contains(std::string(id));
	}

	bool SystemRegistry::ValidateConfig(
		std::string_view id,
		const Ludus::Serialization::LSceneValue& config,
		std::vector<SceneLoadError>& errors) const
	{
		const auto found = _entries.find(std::string(id));
		return found != _entries.end() && found->second.validate(config, errors);
	}

	bool SystemRegistry::Create(
		std::string_view id,
		Ludus::ECS::World& world,
		const Ludus::Serialization::LSceneValue& config) const
	{
		const auto found = _entries.find(std::string(id));
		if (found == _entries.end())
			return false;

		found->second.create(world, config);
		return true;
	}
}
