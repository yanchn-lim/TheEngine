#pragma once

#include <cassert>
#include <utility>
#include <string>
#include "ECS/ecs.hpp"

namespace Engine
{

	//simple helper stuff to query and keep record of the entity

	class Entity
	{
	public:
		Entity() = default;
		Entity(ECS::EntityId id, ECS::World* world,std::string name)
			: m_Id(id)
			, m_World(world)
			, m_Name(name) {}


		bool IsValid() const
		{
			return	m_World &&
				m_Id != ECS::kInvalidEntity &&
				m_World->IsAlive(m_Id);
		}

		explicit operator bool() const { return IsValid(); }

		ECS::EntityId GetId() const { return m_Id; }
		ECS::World* GetWorld() const { return m_World; }

		// ===== HELPERS =====
		template<typename ComponentT, typename... Args>
		ComponentT& AddComponent(Args&&... args)
		{
			assert(IsValid() && "AddComponent on invalid Entity");
			return m_World->AddComponent<ComponentT>(
				m_Id,
				std::forward<Args>(args)...
			);
		}

		template<typename ComponentT>
		bool HasComponent() const
		{
			if (!IsValid()) return false;

			return m_World->HasComponent<ComponentT>(m_Id);
		}

		template<typename ComponentT>
		ComponentT* GetComponent() 
		{
			if (!IsValid()) return nullptr;
			return m_World->GetComponent<ComponentT>(m_Id);
		}

		template<typename ComponentT>
		const ComponentT* GetComponent() const
		{
			if (!IsValid()) return nullptr;
			return m_World->GetComponent<ComponentT>(m_Id);
		}

		template<typename ComponentT>
		void RemoveComponent()
		{
			if (!IsValid()) return;
			m_World->RemoveComponent<ComponentT>(m_Id);
		}

		void Destroy()
		{
			if (IsValid())
			{
				m_World->DestroyEntity(m_Id);
			}
			m_Id = ECS::kInvalidEntity;
			m_World = nullptr;
		}

		// ===== COMPARISON =====
		bool operator==(const Entity& other) const
		{
			return m_Id == other.m_Id && m_World == other.m_World;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		ECS::EntityId m_Id = ECS::kInvalidEntity;
		ECS::World* m_World = nullptr;
		std::string m_Name;
	};
}