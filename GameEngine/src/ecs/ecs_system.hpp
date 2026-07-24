#pragma once

#include <type_traits>

namespace ECS
{
	enum class SystemPhase : uint8_t
	{
		Initialization,
		Update,
		LateUpdate,
		Render
	};


	class World;

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

	//every 
	template<typename... Components>
	struct ComponentList
	{
		static_assert(sizeof...(Components) > 0, "ComponentList requires at least one component type");
		static_assert(
			(std::is_same_v<Components, std::remove_cvref_t<Components>> && ...),
			"Component types must not be const, volatile or references");
	};
}