#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <utility>

// ===== NEW SYSTEM =====
/*
	--- ARCHETYPE
*/	


namespace Engine
{
	namespace ECS
	{
		using EntityId = std::uint32_t;
		constexpr EntityId kInvalidEntity = 0;

		//contains a list of live entities
		//map of EntityId to index
		class World
		{
		public:
			World() : m_NextEntityId(1) //start first entity at 1
			{

			}

			//===== ENTITY API =====
			EntityId CreateEntity()
			{
				EntityId id = m_NextEntityId++;
				m_Entities.push_back(id);
				m_EntityIndex[id] = static_cast<std::uint32_t>(m_Entities.size() - 1);
				return id;
			}

			void DestroyEntity(EntityId entity)
			{
				if (!IsAlive(entity)) return;

				//remove from entity list
				std::uint32_t index = m_EntityIndex[entity];
				EntityId last = m_Entities.back();

				m_Entities[index] = last;
				m_EntityIndex[last] = index;

				m_Entities.pop_back();
				m_EntityIndex.erase(entity);

				//remove component attached to the entity
				for (auto& pair : m_ComponentStorages)
				{
					pair.second->Remove(entity);
				}
			}

			bool IsAlive(EntityId entity) const
			{
				return m_EntityIndex.find(entity) != m_EntityIndex.end();
			}

			//===== COMPONENT API =====
			template<typename ComponentT, typename... Args>
			ComponentT& AddComponent(EntityId entity, Args&... args)
			{
				ComponentStorage<ComponentT>* storage = GetOrCreateStorage<ComponentT>();
				auto it = storage->m_Components.find(entity);

				if (it == storage->m_Components.end())
				{
					auto result = storage->m_Components.emplace(
						entity,
						ComponentT{ std::forward<Args>(args)... }
					);

					return result.first->second;
				}
				else
				{
					it->second = ComponentT{ std::forward<Args>(args)... };
					return it->second;
				}

			}

			template<typename ComponentT>
			void RemoveComponent(EntityId entity)
			{
				auto it = m_ComponentStorages.find(std::type_index(typeid(ComponentT)));
				if (it == m_ComponentStorages.end())
					return;

				ComponentStorage<ComponentT>* storage =
					static_cast<ComponentStorage<ComponentT>*>(it->second.get());

				storage->m_Components.erase(entity);
			}

			template<typename ComponentT>
			bool HasComponent(EntityId entity) const
			{
				auto it = m_ComponentStorages.find(std::type_index(typeid(ComponentT)));
				if (it == m_ComponentStorages.end())
					return false;

				const ComponentStorage<ComponentT>* storage =
					static_cast<const ComponentStorage<ComponentT>*>(it->second.get());

				return storage->m_Components.find(entity) != storage->m_Components.end();
			}

			template<typename ComponentT>
			ComponentT* GetComponent(EntityId entity)
			{
				auto it = m_ComponentStorages.find(std::type_index(typeid(ComponentT)));
				if (it == m_ComponentStorages.end())
					return nullptr;

				ComponentStorage<ComponentT>* storage =
					static_cast<ComponentStorage<ComponentT>*>(it->second.get());

				auto compIt = storage->m_Components.find(entity);
				if (compIt == storage->m_Components.end())
					return nullptr;

				return &compIt->second;
			}

			template<typename ComponentT>
			const ComponentT* GetComponent(EntityId entity) const
			{
				auto it = m_ComponentStorages.find(std::type_index(typeid(ComponentT)));
				if (it == m_ComponentStorages.end())
					return nullptr;

				const ComponentStorage<ComponentT>* storage =
					static_cast<const ComponentStorage<ComponentT>*>(it->second.get());

				auto compIt = storage->m_Components.find(entity);
				if (compIt == storage->m_Components.end())
					return nullptr;

				return &compIt->second;
			}

			//===== ITERATION HELPER =====
			template<typename... Components, typename Func>
			void ForEach(Func&& func) //naive iteration
			{
				for (EntityId entity : m_Entities)
				{
					if ((HasComponent<Components>(entity) && ...))
					{
						func(entity, *GetComponent<Components>(entity)...);
					}
				}
			}

		private:
			struct IComponentStorage
			{
				virtual ~IComponentStorage() = default;
				virtual void Remove(EntityId entity) = 0;
			};

			//container for storing components
			template<typename ComponentT>
			struct ComponentStorage : IComponentStorage
			{
				//key : entity, value : components the entity have
				std::unordered_map<EntityId, ComponentT> m_Components;

				void Remove(EntityId entity) override
				{
					m_Components.erase(entity);
				}
			};

			template<typename ComponentT>
			ComponentStorage<ComponentT>* GetOrCreateStorage()
			{
				std::type_index type = std::type_index(typeid(ComponentT));
				//if there is no map of storage, we create one
				auto it = m_ComponentStorages.find(type); // it = iterator
				if (it == m_ComponentStorages.end())
				{
					auto storage = std::make_unique<ComponentStorage<ComponentT>>();
					ComponentStorage<ComponentT>* ptr = storage.get();
					m_ComponentStorages.emplace(type, std::move(storage));
					return ptr;
				}

				return static_cast<ComponentStorage<ComponentT>*>(it->second.get());
			}


		private:
			EntityId m_NextEntityId;

			//entity management
			std::vector<EntityId> m_Entities;
			std::unordered_map<EntityId, std::uint32_t> m_EntityIndex;
			
			//component type -> storage mapping
			std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> m_ComponentStorages;
		};
	}
}