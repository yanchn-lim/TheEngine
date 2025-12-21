#pragma once

#include "ECS/ecs.hpp"

namespace Engine
{
	class Scene
	{
	public:
		Scene() = default;
		~Scene() = default;

		ECS::World& GetWorld() { return m_World; }
		const ECS::World& GetWorld() const { return m_World; }

		//create entity, etc...
		ECS::EntityId CreateEntity();
		void DestroyEntity(ECS::EntityId entity);

		template<typename ComponentT, typename... Args>
		ComponentT& AddComponent(ECS::EntityId entity, Args&&... args)
		{
			return m_World.AddComponent<ComponentT>(
				entity, 
				std::forward<Args>(args)...
			);
		}

		template<typename ComponentT>
		bool HasComponent(ECS::EntityId entity)const
		{
			return m_World.HasComponent<ComponentT>(entity);
		}

		template<typename... Components, typename Func>
		void ForEach(Func&& func)
		{
			m_World.ForEach<Components...>(std::forward<Func>(func));
		}

	private:
		ECS::World m_World;
	};
}