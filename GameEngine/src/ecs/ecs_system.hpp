#pragma once

#include <type_traits>
#include <typeindex>
#include <string>
#include <memory>

namespace Ludus::ECS
{
	class World;

	// the phase is a sort key for both update paths.
	// it does not select which callback a system receives.
	enum class SystemPhase : uint8_t
	{
		PREUPDATE,
		UPDATE,
		LATEUPDATE,
		RENDER
	};

	// World owns each system and supplies itself to every lifecycle callback.
	class ISystem
	{
	public:
		ISystem() = default;
		virtual ~ISystem() = default;

		// a stable address keeps references to a World-owned system valid.
		ISystem(const ISystem&) = delete;
		ISystem& operator=(const ISystem&) = delete;
		ISystem(ISystem&&) = delete;
		ISystem& operator=(ISystem&&) = delete;

		virtual void OnCreate(World&){}
		virtual void OnUpdate(World&) {}
		virtual void OnFixedUpdate(World&, double) {}
		virtual void OnDestroy(World&) noexcept {}
	};

	struct SystemEntry
	{
		SystemPhase phase;
		int order;
		std::size_t insertionIndex;
		const char* profileName;
		std::unique_ptr<ISystem> instance;
	};
}
