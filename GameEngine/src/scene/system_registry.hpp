#pragma once

#include <concepts>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "ecs/ecs_world.hpp"
#include "scene_load_error.hpp"
#include "serialization/lscene_value.hpp"

namespace Ludus
{
	template<typename System>
	struct SceneSystemCodec;

	// stable ids connect serialized definitions to validators and ECS system
	// factories.
	class SystemRegistry
	{
	public:
		using Validator = std::function<bool(
			const Ludus::Serialization::LSceneValue&,
			std::vector<SceneLoadError>&)>;

		using Factory = std::function<void(
			Ludus::ECS::World&,
			const Ludus::Serialization::LSceneValue&)>;

		bool Register(std::string id, Factory factory);
		bool Register(std::string id, Validator validator, Factory factory);

		template<typename System, typename... Dependencies>
		bool Register(Dependencies&... dependencies)
		{
			static_assert(
				std::derived_from<System, Ludus::ECS::ISystem>,
				"Registered scene systems must derive from Ludus::ECS::ISystem");

			using Codec = SceneSystemCodec<System>;
			static_assert(
				requires { { Codec::Id } -> std::convertible_to<std::string_view>; },
				"SceneSystemCodec<System> must define a stable Id");
			static_assert(
				requires(const Ludus::Serialization::LSceneValue& config,
					std::vector<SceneLoadError>& errors)
				{
					{ Codec::Validate(config, errors) } -> std::same_as<bool>;
				},
				"SceneSystemCodec<System>::Validate has an invalid signature");
			static_assert(
				requires(Ludus::ECS::World& world,
					const Ludus::Serialization::LSceneValue& config,
					Dependencies&... values)
				{
					{ Codec::Create(world, config, values...) } -> std::same_as<void>;
				},
				"SceneSystemCodec<System>::Create does not accept these dependencies");

			// the registry borrows these dependencies. they must outlive every
			// later Create call.
			const auto dependencyPointers =
				std::tuple<Dependencies*...>{ &dependencies... };
			return Register(
				std::string(Codec::Id),
				[](const Ludus::Serialization::LSceneValue& config,
					std::vector<SceneLoadError>& errors)
				{
					return Codec::Validate(config, errors);
				},
				[dependencyPointers](Ludus::ECS::World& world,
					const Ludus::Serialization::LSceneValue& config)
				{
					std::apply(
						[&](auto*... values)
						{
							Codec::Create(world, config, *values...);
						},
						dependencyPointers);
				});
		}

		bool Contains(std::string_view id) const;
		std::vector<std::string> GetIds() const;
		bool CreateDefaultConfig(
			std::string_view id,
			Ludus::Serialization::LSceneValue& output,
			std::vector<SceneLoadError>& errors) const;
		bool ValidateConfig(
			std::string_view id,
			const Ludus::Serialization::LSceneValue& config,
			std::vector<SceneLoadError>& errors) const;
		bool Create(
			std::string_view id,
			Ludus::ECS::World& world,
			const Ludus::Serialization::LSceneValue& config) const;

	private:
		struct Entry
		{
			Validator validate;
			Factory create;
		};

		std::unordered_map<std::string, Entry> _entries;
	};
}
