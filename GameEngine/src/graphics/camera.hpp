#pragma once

#include <glm/glm.hpp>

namespace Graphics
{
	// produces the view and projection matrices passed through FrameConstants
	struct Camera2D
	{
		glm::vec2 position{ 0.f };
		float zoom{ 1.f };
		float rotation{ 0.f };
		float aspectRatio{ 16.f / 9.f };

		void SetViewport(float width, float height);
		glm::mat4 GetView() const;
		glm::mat4 GetProjection() const;
	};
}
