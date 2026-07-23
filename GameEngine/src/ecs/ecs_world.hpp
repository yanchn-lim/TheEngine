#pragma once

namespace ECS
{
	class World
	{
	private:
		uint32_t _entityCounter = 0;
		std::vector<uint32_t> _entities;

	public:
		World() = default;
		~World() = default;

		uint32_t CreateEntity();
		uint32_t GetEntityCount() const;
		void RemoveEntity(uint32_t entityId);
	};
}