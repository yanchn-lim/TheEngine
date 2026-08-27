#include "input.hpp"

namespace Ludus
{
	void Input::BeginFrame() noexcept
	{
		for (ButtonState& state : _keys)
		{
			state.pressed = false;
			state.released = false;
		}
		for (ButtonState& state : _mouseButtons)
		{
			state.pressed = false;
			state.released = false;
		}
		_mouseDelta = {};
		_scrollDelta = {};
		_events.clear();
	}

	bool Input::IsKeyDown(Key key) const noexcept
	{
		const int index = static_cast<int>(key);
		return index >= 0 && static_cast<size_t>(index) < _keys.size() &&
			_keys[static_cast<size_t>(index)].down;
	}

	bool Input::WasKeyPressed(Key key) const noexcept
	{
		const int index = static_cast<int>(key);
		return index >= 0 && static_cast<size_t>(index) < _keys.size() &&
			_keys[static_cast<size_t>(index)].pressed;
	}

	bool Input::WasKeyReleased(Key key) const noexcept
	{
		const int index = static_cast<int>(key);
		return index >= 0 && static_cast<size_t>(index) < _keys.size() &&
			_keys[static_cast<size_t>(index)].released;
	}

	bool Input::IsMouseButtonDown(MouseButton button) const noexcept
	{
		const int index = static_cast<int>(button);
		return index >= 0 && static_cast<size_t>(index) < _mouseButtons.size() &&
			_mouseButtons[static_cast<size_t>(index)].down;
	}

	bool Input::WasMouseButtonPressed(MouseButton button) const noexcept
	{
		const int index = static_cast<int>(button);
		return index >= 0 && static_cast<size_t>(index) < _mouseButtons.size() &&
			_mouseButtons[static_cast<size_t>(index)].pressed;
	}

	bool Input::WasMouseButtonReleased(MouseButton button) const noexcept
	{
		const int index = static_cast<int>(button);
		return index >= 0 && static_cast<size_t>(index) < _mouseButtons.size() &&
			_mouseButtons[static_cast<size_t>(index)].released;
	}

	glm::vec2 Input::GetMousePosition() const noexcept
	{
		return _mousePosition;
	}

	glm::vec2 Input::GetMouseDelta() const noexcept
	{
		return _mouseDelta;
	}

	glm::vec2 Input::GetScrollDelta() const noexcept
	{
		return _scrollDelta;
	}

	Modifier Input::GetModifiers() const noexcept
	{
		return _modifiers;
	}

	bool Input::IsFocused() const noexcept
	{
		return _focused;
	}

	std::span<const InputEvent> Input::GetEvents() const noexcept
	{
		return _events;
	}

	void Input::ProcessKey(Key key, InputAction action, Modifier modifiers)
	{
		const int index = static_cast<int>(key);
		if (index < 0 || static_cast<size_t>(index) >= _keys.size())
			return;

		ButtonState& state = _keys[static_cast<size_t>(index)];
		if (action == InputAction::Press)
		{
			state.pressed = !state.down;
			state.down = true;
		}
		else if (action == InputAction::Release)
		{
			state.released = state.down;
			state.down = false;
		}
		else
		{
			state.down = true;
		}
		_modifiers = modifiers;
		_events.push_back({ InputEventType::Key, key, MouseButton::Left,
			action, modifiers });
	}

	void Input::ProcessMouseButton(
		MouseButton button,
		InputAction action,
		Modifier modifiers)
	{
		const int index = static_cast<int>(button);
		if (index < 0 || static_cast<size_t>(index) >= _mouseButtons.size())
			return;

		ButtonState& state = _mouseButtons[static_cast<size_t>(index)];
		if (action == InputAction::Press)
		{
			state.pressed = !state.down;
			state.down = true;
		}
		else if (action == InputAction::Release)
		{
			state.released = state.down;
			state.down = false;
		}
		_modifiers = modifiers;
		InputEvent event{ InputEventType::MouseButton };
		event.mouseButton = button;
		event.action = action;
		event.modifiers = modifiers;
		_events.push_back(event);
	}

	void Input::ProcessMouseMove(glm::vec2 position)
	{
		if (_hasMousePosition)
			_mouseDelta += position - _mousePosition;
		_mousePosition = position;
		_hasMousePosition = true;
		InputEvent event{ InputEventType::MouseMove };
		event.value = position;
		_events.push_back(event);
	}

	void Input::ProcessScroll(glm::vec2 delta)
	{
		_scrollDelta += delta;
		InputEvent event{ InputEventType::Scroll };
		event.value = delta;
		_events.push_back(event);
	}

	void Input::ProcessFocus(bool focused)
	{
		_focused = focused;
		if (!focused)
		{
			for (ButtonState& state : _keys)
			{
				state.released = state.released || state.down;
				state.down = false;
			}
			for (ButtonState& state : _mouseButtons)
			{
				state.released = state.released || state.down;
				state.down = false;
			}
			_modifiers = Modifier::None;
			_hasMousePosition = false;
		}
		InputEvent event{ InputEventType::Focus };
		event.focused = focused;
		_events.push_back(event);
	}
}
