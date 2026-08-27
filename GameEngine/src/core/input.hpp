#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include <glm/vec2.hpp>

namespace Ludus::Platform
{
	class Window;
}

namespace Tests
{
	bool RunInputTests();
}

namespace Ludus
{
	enum class Key : int
	{
		Unknown = -1,
		Space = 32,
		Apostrophe = 39,
		Comma = 44,
		Minus = 45,
		Period = 46,
		Slash = 47,
		Digit0 = 48,
		Digit1,
		Digit2,
		Digit3,
		Digit4,
		Digit5,
		Digit6,
		Digit7,
		Digit8,
		Digit9,
		Semicolon = 59,
		Equal = 61,
		A = 65,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,
		LeftBracket = 91,
		Backslash,
		RightBracket,
		GraveAccent = 96,
		Escape = 256,
		Enter,
		Tab,
		Backspace,
		Insert,
		Delete,
		Right,
		Left,
		Down,
		Up,
		PageUp,
		PageDown,
		Home,
		End,
		CapsLock = 280,
		ScrollLock,
		NumLock,
		PrintScreen,
		Pause,
		F1 = 290,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		LeftShift = 340,
		LeftControl,
		LeftAlt,
		LeftSuper,
		RightShift,
		RightControl,
		RightAlt,
		RightSuper,
		Menu,
		Count
	};

	enum class MouseButton : int
	{
		Left = 0,
		Right = 1,
		Middle = 2,
		Button4,
		Button5,
		Button6,
		Button7,
		Button8,
		Count
	};

	enum class Modifier : uint8_t
	{
		None = 0,
		Shift = 1 << 0,
		Control = 1 << 1,
		Alt = 1 << 2,
		Super = 1 << 3,
		CapsLock = 1 << 4,
		NumLock = 1 << 5
	};

	constexpr Modifier operator|(Modifier left, Modifier right) noexcept
	{
		return static_cast<Modifier>(
			static_cast<uint8_t>(left) | static_cast<uint8_t>(right));
	}

	enum class InputAction
	{
		Press,
		Release,
		Repeat
	};

	enum class InputEventType
	{
		Key,
		MouseButton,
		MouseMove,
		Scroll,
		Focus
	};

	struct InputEvent
	{
		InputEventType type;
		Key key = Key::Unknown;
		MouseButton mouseButton = MouseButton::Left;
		InputAction action = InputAction::Press;
		Modifier modifiers = Modifier::None;
		glm::vec2 value{};
		bool focused = true;
	};

	class Input
	{
	public:
		void BeginFrame() noexcept;

		bool IsKeyDown(Key key) const noexcept;
		bool WasKeyPressed(Key key) const noexcept;
		bool WasKeyReleased(Key key) const noexcept;
		bool IsMouseButtonDown(MouseButton button) const noexcept;
		bool WasMouseButtonPressed(MouseButton button) const noexcept;
		bool WasMouseButtonReleased(MouseButton button) const noexcept;

		glm::vec2 GetMousePosition() const noexcept;
		glm::vec2 GetMouseDelta() const noexcept;
		glm::vec2 GetScrollDelta() const noexcept;
		Modifier GetModifiers() const noexcept;
		bool IsFocused() const noexcept;
		std::span<const InputEvent> GetEvents() const noexcept;

	private:
		struct ButtonState
		{
			bool down = false;
			bool pressed = false;
			bool released = false;
		};

		void ProcessKey(Key key, InputAction action, Modifier modifiers);
		void ProcessMouseButton(
			MouseButton button,
			InputAction action,
			Modifier modifiers);
		void ProcessMouseMove(glm::vec2 position);
		void ProcessScroll(glm::vec2 delta);
		void ProcessFocus(bool focused);

		static constexpr size_t KeyCount = static_cast<size_t>(Key::Count);
		static constexpr size_t MouseButtonCount =
			static_cast<size_t>(MouseButton::Count);

		std::array<ButtonState, KeyCount> _keys{};
		std::array<ButtonState, MouseButtonCount> _mouseButtons{};
		glm::vec2 _mousePosition{};
		glm::vec2 _mouseDelta{};
		glm::vec2 _scrollDelta{};
		Modifier _modifiers = Modifier::None;
		bool _focused = true;
		bool _hasMousePosition = false;
		std::vector<InputEvent> _events;

		friend class Ludus::Platform::Window;
		friend bool Tests::RunInputTests();
	};
}
