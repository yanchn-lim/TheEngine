#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Ludus::Graphics
{
	enum class ProjectionMode
	{
		Perspective,
		Orthographic
	};

	// produces right-handed view matrices with zero-to-one projection depth.
	// orthographicSize is the full vertical span.
	// callers own and normalize the rotation quaternion.
	struct Camera
	{
		glm::vec3 position{ 0.0f, 1.5f, 5.0f };
		glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
		ProjectionMode projectionMode = ProjectionMode::Perspective;
		float verticalFieldOfViewDegrees = 60.0f;
		float orthographicSize = 10.0f;
		float nearPlane = 0.05f;
		float farPlane = 1000.0f;
		float aspectRatio = 16.0f / 9.0f;

		void SetViewport(float width, float height);
		glm::mat4 GetView() const;
		glm::mat4 GetProjection() const;
	};
}
