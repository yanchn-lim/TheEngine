#pragma once
#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>

#include "time.hpp"

namespace Engine
{
	struct FrameStartEvent
	{
		const Time& time;
	};

	struct FixedUpdateEvent
	{
		const Time& time;
		float fixedDeltaTime;
		int stepIndex;
	};

	struct UpdateEvent
	{
		const Time& time;
	};

	struct RenderEvent
	{
		const Time& time;
		float alpha;
	};

	struct FrameEndEvent
	{
		const Time& time;
	};

	//=====Event Bus=====

	//event listener alias
	template<typename EventT> //generic event type
	//alias function<const EventT&> as EventListener
	//function<void(...)> is similar to Action in C#
	using EventListener = std::function<void(const EventT&)>; //type alias

	//handles event calling and subscribing
	class EventBus
	{
	public:
		template<typename EventT>
		void Subscribe(EventListener<EventT> listener)
		{
			auto& list = GetOrCreateList<EventT>(); //find or create a list of events
			//move() is not copying but instead changing an expr 
			//from being an l value to a rvalue
			//allows swapping data without making copies
			//original memory addr of move's source is now freed to be used
			list.listeners.push_back(std::move(listener)); //store the listener in the event
		}

		template<typename EventT>
		void Emit(const EventT& ev) //invoke the event
		{
			//type_index lets us use type info as a value
			const auto type = std::type_index(typeid(EventT)); //get the type of event
			auto it = m_Listeners.find(type); //get the kvp
			if (it == m_Listeners.end()) return;

			auto* base = it->second.get(); //get the raw pointer
			auto* list = static_cast<ListenerList<EventT>*>(base); //cast as ListenerList

			for (auto& listener : list->listeners) //similar to foreach
			{
				listener(ev);//invoke all listeners
			}
		}

	private:
		//base class to handle different type of events
		struct ListenerListBase
		{
			virtual ~ListenerListBase() = default; //deconstructor for children of this type
		};

		template<typename EventT>
		struct ListenerList : ListenerListBase
		{
			std::vector<EventListener<EventT>> listeners; // listeners to that event
		};

		//store a map of the event and their listeners
		//unique_ptr :
		//	-smart pointer that owns a heap allocation
		//	-automatically destroys object when out of scope or erased from the map
		//	-enforces single ownership
		//	-cannot be copied
		std::unordered_map<std::type_index, std::unique_ptr<ListenerListBase>> m_Listeners;

		//create a list if it doesnt exist
		template<typename EventT>
		ListenerList<EventT>& GetOrCreateList()
		{
			const auto type = std::type_index(typeid(EventT)); //get the type key
			auto it = m_Listeners.find(type); // try to find in the map

			if (it == m_Listeners.end()) //if it doesnt exist
			{
				auto ptr = std::make_unique<ListenerList<EventT>>(); //allocates on heap
				auto* raw = ptr.get(); //get raw ptr
				m_Listeners.emplace(type, std::move(ptr)); //insert raw ptr into map, HAVE TO USE MOVE for unique ptr
				return *raw;
			}

			return *static_cast<ListenerList<EventT>*>(it->second.get());
		}
	};
}