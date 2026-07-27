#pragma once

#include <type_traits>
#include <typeindex>
#include <string>
#include <memory>

namespace ECS
{
	class World;

	enum class SystemPhase : uint8_t
	{
		PREUPDATE,
		UPDATE,
		LATEUPDATE,
		RENDER
	};

	//future system classes inherits this
	class ISystem
	{
	public:
		ISystem() = default;
		virtual ~ISystem() = default;

		//prevent any copying/moving
		ISystem(const ISystem&) = delete;
		ISystem& operator=(const ISystem&) = delete;
		ISystem(ISystem&&) = delete;
		ISystem& operator=(ISystem&&) = delete;

		virtual void OnCreate(World&){}
		virtual void OnUpdate(World&) = 0;
		virtual void OnDestroy(World&){}
	};

	struct SystemEntry
	{
		SystemPhase phase;
		int order;
		std::size_t insertionIndex;
		std::unique_ptr<ISystem> instance;
	};
}