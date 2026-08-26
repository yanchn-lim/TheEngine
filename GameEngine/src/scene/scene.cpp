#include "scene.hpp"

namespace Ludus
{
	ECS::Entity Scene::CreateEntity(std::string id, std::string name)
	{
		if (id.empty())
			return {};

		const auto existing = _entities.find(id);
		if (existing != _entities.end())
		{
			if (_world.IsEntityAlive(existing->second.entity))
				return {};
			_entities.erase(existing);
		}

		const ECS::Entity entity = _world.CreateEntity();
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

	ECS::Entity Scene::FindEntity(std::string_view id) const
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
		Serialization::LSceneValue assets,
		SceneAssetContext assetContext)
	{
		_name = std::move(name);
		_assets = std::move(assets);
		_assetContext = std::move(assetContext);
	}

	std::string_view Scene::GetName() const noexcept
	{
		return _name;
	}

	const Serialization::LSceneValue& Scene::GetAssetDeclarations() const noexcept
	{
		return _assets;
	}

	const SceneAssetContext& Scene::GetAssetContext() const noexcept
	{
		return _assetContext;
	}

	void Scene::Swap(Scene& other) noexcept
	{
		_world.Swap(other._world);
		_entities.swap(other._entities);
		_name.swap(other._name);
		std::swap(_assets, other._assets);
		std::swap(_assetContext, other._assetContext);
	}

	ECS::World& Scene::GetWorld() noexcept
	{
		return _world;
	}

	const ECS::World& Scene::GetWorld() const noexcept
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
