#pragma once

#include <cstdint>
#include <cstddef>
#include <typeindex>
#include <unordered_map>
#include <array>
#include <cassert>

namespace Engine
{
	namespace ECS2
	{
		using TypeId = std::uint8_t;
		using Signature = std::uint64_t;

		inline constexpr std::size_t kMaxComponentTypes = 64;

		inline constexpr Signature Bit(TypeId id)
		{
			return Signature{ 1 } << id;
		}

		struct ComponentInfo
		{
			TypeId      id{};
			std::size_t size{};
			std::size_t align{};
		};

        class ComponentRegistry
        {
        public:
            static ComponentRegistry& Instance()
            {
                static ComponentRegistry r;
                return r;
            }

            template<typename T>
            TypeId Type()
            {
                const std::type_index key(typeid(T));

                auto it = m_TypeToId.find(key);
                if (it != m_TypeToId.end())
                    return it->second;

                assert(m_NextTypeId < kMaxComponentTypes && "ECS2: exceeded 64 component types");

                const TypeId id = static_cast<TypeId>(m_NextTypeId++);
                m_TypeToId.emplace(key, id);

                m_Info[id] = ComponentInfo{ id, sizeof(T), alignof(T) };
                m_Used[id] = true;

                return id;
            }

            template<typename T>
            const ComponentInfo& Info()
            {
                const TypeId id = Type<T>();
                return m_Info[id];
            }

            const ComponentInfo& Info(TypeId id) const
            {
                assert(id < kMaxComponentTypes && m_Used[id] && "ECS2: invalid/unregistered TypeId");
                return m_Info[id];
            }

        private:
            std::unordered_map<std::type_index, TypeId> m_TypeToId;

            std::array<ComponentInfo, kMaxComponentTypes> m_Info{};
            std::array<bool, kMaxComponentTypes> m_Used{};

            std::size_t m_NextTypeId = 0;
        };

        template<typename T>
        inline TypeId TypeIdOf()
        {
            return ComponentRegistry::Instance().Type<T>();
        }

        template<typename T>
        inline Signature SigOf()
        {
            return Bit(TypeIdOf<T>());
        }

        template<typename T>
        inline const ComponentInfo& InfoOf()
        {
            return ComponentRegistry::Instance().Info<T>();
        }
	}
}