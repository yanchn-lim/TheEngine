#include "scene.hpp"

#include <cstdio>
#include <random>

namespace Ludus
{
	namespace
	{
		std::string GenerateEntityId()
		{
			thread_local std::mt19937_64 generator(std::random_device{}());
			char suffix[33]{};
			std::snprintf(
				suffix,
				sizeof(suffix),
				"%016llx%016llx",
				static_cast<unsigned long long>(generator()),
				static_cast<unsigned long long>(generator()));
			return "entity_" + std::string(suffix);
		}
	}

	std::string Scene::CreateEntity(std::string name)
	{
		for (size_t attempt = 0; attempt < 100; ++attempt)
		{
			// RestoreEntity provides the collision check and creates the ECS entity.
			std::string id = GenerateEntityId();
			if (RestoreEntity(id, name).IsValid())
				return id;
		}
		return {};
	}

	Ludus::ECS::Entity Scene::RestoreEntity(std::string id, std::string name)
	{
		if (id.empty())
			return {};

		const auto existing = _entities.find(id);
		if (existing != _entities.end())
		{
			// a direct World removal can leave a dead scene record.
			if (_world.IsEntityAlive(existing->second.entity))
				return {};
			_entities.erase(existing);
		}

		const Ludus::ECS::Entity entity = _world.CreateEntity();
		_entities.emplace(std::move(id), EntityRecord{ entity, std::move(name) });
		return entity;
	}

	bool Scene::RemoveEntity(std::string_view id)
	{
		const auto found = _entities.find(std::string(id));
		if (found == _entities.end())
			return false;

		_world.RemoveEntity(found->second.entity);
		_entities.erase(found);
		return true;
	}

	Ludus::ECS::Entity Scene::FindEntity(std::string_view id) const
	{
		const auto found = _entities.find(std::string(id));
		if (found == _entities.end() || !_world.IsEntityAlive(found->second.entity))
			return {};
		return found->second.entity;
	}

	std::string_view Scene::GetEntityName(std::string_view id) const
	{
		const auto found = _entities.find(std::string(id));
		if (found == _entities.end() || !_world.IsEntityAlive(found->second.entity))
			return {};
		return found->second.name;
	}

	const std::unordered_map<std::string, Scene::EntityRecord>& Scene::GetEntities() const noexcept
	{
		return _entities;
	}

	void Scene::SetSerializationData(
		std::string name,
		Ludus::Serialization::LSceneValue assets,
		SceneAssetContext assetContext,
		std::vector<SceneSystemDefinition> systems)
	{
		_name = std::move(name);
		_assets = std::move(assets);
		_assetContext = std::move(assetContext);
		_systems = std::move(systems);
	}

	std::string_view Scene::GetName() const noexcept
	{
		return _name;
	}

	const Ludus::Serialization::LSceneValue& Scene::GetAssetDeclarations() const noexcept
	{
		return _assets;
	}

	const SceneAssetContext& Scene::GetAssetContext() const noexcept
	{
		return _assetContext;
	}

	const std::vector<SceneSystemDefinition>& Scene::GetSystems() const noexcept
	{
		return _systems;
	}

	std::vector<SceneSystemDefinition>& Scene::GetSystems() noexcept
	{
		return _systems;
	}

	void Scene::SetSystems(std::vector<SceneSystemDefinition> systems)
	{
		_systems = std::move(systems);
	}

	void Scene::Swap(Scene& other) noexcept
	{
		_world.Swap(other._world);
		_entities.swap(other._entities);
		_name.swap(other._name);
		std::swap(_assets, other._assets);
		std::swap(_assetContext, other._assetContext);
		_systems.swap(other._systems);
	}

	Ludus::ECS::World& Scene::GetWorld() noexcept
	{
		return _world;
	}

	const Ludus::ECS::World& Scene::GetWorld() const noexcept
	{
		return _world;
	}

	void Scene::FixedUpdate(double fixedDeltaTime)
	{
		_world.FixedUpdateSystems(fixedDeltaTime);
	}

	void Scene::Update()
	{
		_world.UpdateSystems();
	}


}
