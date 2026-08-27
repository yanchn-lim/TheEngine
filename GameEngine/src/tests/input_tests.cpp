#include "input_tests.hpp"

#include "core/action_map.hpp"
#include "core/input.hpp"

namespace Tests
{
	namespace
	{
		enum class TestAction
		{
			Undo,
			Fire,
			Count
		};
	}

	bool RunInputTests()
	{
		Ludus::Input input;
		input.BeginFrame();
		input.ProcessKey(
			Ludus::Key::W,
			Ludus::InputAction::Press,
			Ludus::Modifier::Control);
		input.ProcessKey(
			Ludus::Key::W,
			Ludus::InputAction::Repeat,
			Ludus::Modifier::Control);
		if (!input.IsKeyDown(Ludus::Key::W) ||
			!input.WasKeyPressed(Ludus::Key::W) ||
			input.WasKeyReleased(Ludus::Key::W) ||
			input.GetModifiers() != Ludus::Modifier::Control ||
			input.GetEvents().size() != 2)
		{
			return false;
		}

		input.BeginFrame();
		if (!input.IsKeyDown(Ludus::Key::W) ||
			input.WasKeyPressed(Ludus::Key::W) ||
			!input.GetEvents().empty())
		{
			return false;
		}

		input.ProcessMouseMove({ 10.0f, 20.0f });
		input.ProcessMouseMove({ 13.0f, 25.0f });
		input.ProcessScroll({ 0.0f, 1.0f });
		input.ProcessScroll({ 2.0f, -0.5f });
		input.ProcessMouseButton(
			Ludus::MouseButton::Left,
			Ludus::InputAction::Press,
			Ludus::Modifier::Shift);
		if (input.GetMousePosition() != glm::vec2(13.0f, 25.0f) ||
			input.GetMouseDelta() != glm::vec2(3.0f, 5.0f) ||
			input.GetScrollDelta() != glm::vec2(2.0f, 0.5f) ||
			!input.IsMouseButtonDown(Ludus::MouseButton::Left) ||
			!input.WasMouseButtonPressed(Ludus::MouseButton::Left))
		{
			return false;
		}

		input.ProcessFocus(false);
		if (input.IsFocused() || input.IsKeyDown(Ludus::Key::W) ||
			input.IsMouseButtonDown(Ludus::MouseButton::Left) ||
			!input.WasKeyReleased(Ludus::Key::W) ||
			!input.WasMouseButtonReleased(Ludus::MouseButton::Left) ||
			input.GetModifiers() != Ludus::Modifier::None ||
			input.GetEvents().back().type != Ludus::InputEventType::Focus)
		{
			return false;
		}

		input.BeginFrame();
		input.ProcessKey(
			Ludus::Key::W,
			Ludus::InputAction::Release,
			Ludus::Modifier::None);
		if (input.WasKeyReleased(Ludus::Key::W) ||
			input.GetMouseDelta() != glm::vec2{} ||
			input.GetScrollDelta() != glm::vec2{})
		{
			return false;
		}

		Ludus::ActionMap<TestAction> actions;
		if (!actions.Bind(
				TestAction::Undo,
				{ Ludus::Key::Z, Ludus::Modifier::Control }) ||
			actions.Bind(
				TestAction::Undo,
				{ Ludus::Key::Z, Ludus::Modifier::Control }) ||
			!actions.Bind(TestAction::Fire, Ludus::MouseButton::Left))
		{
			return false;
		}

		int pressedCount = 0;
		int releasedCount = 0;
		auto pressed = actions.OnPressed(
			TestAction::Undo, [&pressedCount] { ++pressedCount; });
		auto released = actions.OnReleased(
			TestAction::Undo, [&releasedCount] { ++releasedCount; });

		input.BeginFrame();
		input.ProcessKey(
			Ludus::Key::Z,
			Ludus::InputAction::Press,
			Ludus::Modifier::Control);
		actions.Update(input);
		if (!actions.IsDown(TestAction::Undo) ||
			!actions.WasPressed(TestAction::Undo) ||
			pressedCount != 1)
		{
			return false;
		}

		input.BeginFrame();
		actions.Update(input);
		if (!actions.IsDown(TestAction::Undo) ||
			actions.WasPressed(TestAction::Undo) || pressedCount != 1)
		{
			return false;
		}

		input.ProcessKey(
			Ludus::Key::Z,
			Ludus::InputAction::Release,
			Ludus::Modifier::Control);
		actions.Update(input);
		if (!actions.WasReleased(TestAction::Undo) || releasedCount != 1)
			return false;

		pressed.Disconnect();
		input.BeginFrame();
		input.ProcessKey(
			Ludus::Key::Z,
			Ludus::InputAction::Press,
			Ludus::Modifier::Control);
		actions.Update(input);
		if (pressedCount != 1)
			return false;

		actions.Update(input, true, false);
		if (actions.IsDown(TestAction::Undo) ||
			!actions.WasReleased(TestAction::Undo) ||
			releasedCount != 2)
		{
			return false;
		}

		input.BeginFrame();
		input.ProcessKey(
			Ludus::Key::LeftControl,
			Ludus::InputAction::Press,
			Ludus::Modifier::Control);
		input.ProcessKey(
			Ludus::Key::Z,
			Ludus::InputAction::Press,
			Ludus::Modifier::Control);
		input.ProcessKey(
			Ludus::Key::Z,
			Ludus::InputAction::Release,
			Ludus::Modifier::Control);
		input.ProcessKey(
			Ludus::Key::LeftControl,
			Ludus::InputAction::Release,
			Ludus::Modifier::None);
		actions.Update(input);
		return !actions.IsDown(TestAction::Undo) &&
			actions.WasPressed(TestAction::Undo) &&
			actions.WasReleased(TestAction::Undo) &&
			releasedCount == 3;
	}
}
