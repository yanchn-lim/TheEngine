#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "input.hpp"

namespace Ludus
{
	struct KeyChord
	{
		Key key;
		Modifier modifiers = Modifier::None;

		bool operator==(const KeyChord&) const = default;
	};

	struct ActionState
	{
		bool down = false;
		bool pressed = false;
		bool released = false;
	};

	class ActionConnection
	{
	public:
		ActionConnection() = default;
		explicit ActionConnection(std::function<void()> disconnect)
			: _disconnect(std::move(disconnect))
		{
		}

		~ActionConnection()
		{
			Disconnect();
		}

		ActionConnection(const ActionConnection&) = delete;
		ActionConnection& operator=(const ActionConnection&) = delete;

		ActionConnection(ActionConnection&& other) noexcept
			: _disconnect(std::move(other._disconnect))
		{
		}

		ActionConnection& operator=(ActionConnection&& other) noexcept
		{
			if (this != &other)
			{
				Disconnect();
				_disconnect = std::move(other._disconnect);
			}
			return *this;
		}

		void Disconnect()
		{
			if (!_disconnect)
				return;
			_disconnect();
			_disconnect = {};
		}

	private:
		std::function<void()> _disconnect;
	};

	template<typename Action>
	class ActionMap
	{
		static_assert(std::is_enum_v<Action>, "Action must be an enum");

	public:
		using Callback = std::function<void()>;

		bool Bind(Action action, KeyChord chord)
		{
			if (!IsValid(action) || chord.key == Key::Unknown)
				return false;
			const Binding binding{ action, chord };
			if (std::ranges::any_of(_bindings,
				[&binding](const Binding& existing)
				{
					return existing == binding;
				}))
			{
				return false;
			}
			_bindings.push_back(binding);
			return true;
		}

		bool Bind(
			Action action,
			MouseButton button,
			Modifier modifiers = Modifier::None)
		{
			if (!IsValid(action))
				return false;
			const Binding binding{ action, MouseChord{ button, modifiers } };
			if (std::ranges::any_of(_bindings,
				[&binding](const Binding& existing)
				{
					return existing == binding;
				}))
			{
				return false;
			}
			_bindings.push_back(binding);
			return true;
		}

		void Update(
			const Input& input,
			bool keyboardBlocked = false,
			bool mouseBlocked = false)
		{
			std::array<ActionState, ActionCount> next{};
			for (const Binding& binding : _bindings)
			{
				ActionState& state = next[ToIndex(binding.action)];
				if (const auto* key = std::get_if<KeyChord>(&binding.input))
				{
					if (keyboardBlocked)
						continue;
					state.down = state.down ||
						(ModifiersMatch(input.GetModifiers(), key->modifiers) &&
							input.IsKeyDown(key->key));
					for (const InputEvent& event : input.GetEvents())
					{
						if (event.type != InputEventType::Key || event.key != key->key ||
							!ModifiersMatch(event.modifiers, key->modifiers))
						{
							continue;
						}
						state.pressed = state.pressed ||
							event.action == InputAction::Press;
						state.released = state.released ||
							event.action == InputAction::Release;
					}
				}
				else
				{
					const MouseChord& mouse = std::get<MouseChord>(binding.input);
					if (mouseBlocked)
						continue;
					state.down = state.down ||
						(ModifiersMatch(input.GetModifiers(), mouse.modifiers) &&
							input.IsMouseButtonDown(mouse.button));
					for (const InputEvent& event : input.GetEvents())
					{
						if (event.type != InputEventType::MouseButton ||
							event.mouseButton != mouse.button ||
							!ModifiersMatch(event.modifiers, mouse.modifiers))
						{
							continue;
						}
						state.pressed = state.pressed ||
							event.action == InputAction::Press;
						state.released = state.released ||
							event.action == InputAction::Release;
					}
				}
			}

			for (size_t index = 0; index < ActionCount; ++index)
			{
				next[index].pressed = next[index].pressed ||
					(!_states[index].down && next[index].down);
				next[index].released = next[index].released ||
					(_states[index].down && !next[index].down);
				_states[index] = next[index];
				if (_states[index].pressed)
					Dispatch(static_cast<Action>(index), true);
				if (_states[index].released)
					Dispatch(static_cast<Action>(index), false);
			}
		}

		const ActionState& Get(Action action) const noexcept
		{
			static const ActionState empty;
			return IsValid(action) ? _states[ToIndex(action)] : empty;
		}

		bool IsDown(Action action) const noexcept { return Get(action).down; }
		bool WasPressed(Action action) const noexcept { return Get(action).pressed; }
		bool WasReleased(Action action) const noexcept { return Get(action).released; }

		ActionConnection OnPressed(Action action, Callback callback)
		{
			return Connect(action, true, std::move(callback));
		}

		ActionConnection OnReleased(Action action, Callback callback)
		{
			return Connect(action, false, std::move(callback));
		}

	private:
		struct MouseChord
		{
			MouseButton button;
			Modifier modifiers;

			bool operator==(const MouseChord&) const = default;
		};

		struct Binding
		{
			Action action;
			std::variant<KeyChord, MouseChord> input;

			bool operator==(const Binding&) const = default;
		};

		struct CallbackEntry
		{
			size_t id;
			Action action;
			bool pressed;
			Callback callback;
		};

		struct CallbackRegistry
		{
			size_t nextId = 1;
			std::vector<CallbackEntry> entries;
		};

		static constexpr size_t ActionCount = static_cast<size_t>(Action::Count);
		static constexpr uint8_t ShortcutModifierMask =
			static_cast<uint8_t>(Modifier::Shift) |
			static_cast<uint8_t>(Modifier::Control) |
			static_cast<uint8_t>(Modifier::Alt) |
			static_cast<uint8_t>(Modifier::Super);

		static bool IsValid(Action action) noexcept
		{
			return static_cast<size_t>(action) < ActionCount;
		}

		static size_t ToIndex(Action action) noexcept
		{
			return static_cast<size_t>(action);
		}

		static bool ModifiersMatch(Modifier actual, Modifier required) noexcept
		{
			return (static_cast<uint8_t>(actual) & ShortcutModifierMask) ==
				(static_cast<uint8_t>(required) & ShortcutModifierMask);
		}

		ActionConnection Connect(Action action, bool pressed, Callback callback)
		{
			if (!IsValid(action) || !callback)
				return {};
			const size_t id = _callbacks->nextId++;
			_callbacks->entries.push_back({
				id, action, pressed, std::move(callback) });
			std::weak_ptr<CallbackRegistry> callbacks = _callbacks;
			return ActionConnection([callbacks, id]
			{
				if (const auto locked = callbacks.lock())
				{
					std::erase_if(locked->entries,
						[id](const CallbackEntry& entry)
						{
							return entry.id == id;
						});
				}
			});
		}

		void Dispatch(Action action, bool pressed)
		{
			std::vector<Callback> callbacks;
			for (const CallbackEntry& entry : _callbacks->entries)
				if (entry.action == action && entry.pressed == pressed)
					callbacks.push_back(entry.callback);
			for (const Callback& callback : callbacks)
				callback();
		}

		std::vector<Binding> _bindings;
		std::array<ActionState, ActionCount> _states{};
		std::shared_ptr<CallbackRegistry> _callbacks =
			std::make_shared<CallbackRegistry>();
	};
}
