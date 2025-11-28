#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include "events.hpp"

struct GLFWwindow;

namespace Engine
{
	using KeyCode		= int;
	using MouseButton	= int;

	//set some limits
	constexpr int MaxKeys			= 512;
	constexpr int MaxMouseButtons	= 8;

#pragma region INPUT EVENTS
	//=====INPUT EVENTS=====
	struct KeyPressedEvent
	{
		KeyCode key;
		bool repeated;
	};

	struct KeyReleasedEvent
	{
		KeyCode key;
	};

	//maybe change to a vector struct in the future
	struct MouseButtonPressedEvent
	{
		MouseButton button;
		double x;
		double y;		
	};

	struct MouseButtonReleasedEvent
	{
		MouseButton button;
		double x;
		double y;
	};

	struct MouseMovedEvent
	{
		double x;
		double y;
		double deltaX;
		double deltaY;
	};

	struct MouseScrolledEvent
	{
		double offsetX;
		double offsetY;
	};
#pragma endregion

	struct InputState
	{
		bool keyDown[MaxKeys] = {};
		bool keyPressed[MaxKeys] = {}; // true only on the frame key went from up -> down
		bool keyReleased[MaxKeys] = {}; // true only on the frame key went from down -> up

		bool mouseDown[MaxMouseButtons] = {};
		bool mousePressed[MaxMouseButtons] = {};
		bool mouseReleased[MaxMouseButtons] = {};

		double mouseX = 0.0;
		double mouseY = 0.0;
		double mouseDeltaX = 0.0; // movement since start of frame
		double mouseDeltaY = 0.0;

		double scrollX = 0.0;
		double scrollY = 0.0;
	};


	//=====INPUT SYSTEM=====
	class EventBus; //forward decl

	namespace InputSystem
	{
		void Initialize(GLFWwindow* window, EventBus* eventBus = nullptr);

		void BeginFrame();

		const InputState& GetState();

		bool IsKeyDown(KeyCode key);
		bool WasKeyPressed(KeyCode key);   // true only on the frame key was pressed
		bool WasKeyReleased(KeyCode key);  // true only on the frame key was released

		bool IsMouseButtonDown(MouseButton button);
		bool WasMouseButtonPressed(MouseButton button);
		bool WasMouseButtonReleased(MouseButton button);

		double GetMouseX();
		double GetMouseY();
		double GetMouseDeltaX();
		double GetMouseDeltaY();

		double GetScrollX();
		double GetScrollY();
	}
}