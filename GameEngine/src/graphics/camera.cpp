#include <glm/gtc/matrix_transform.hpp>

#include "camera.hpp"

namespace Ludus::Graphics
{
	void Camera::SetViewport(float width, float height)
	{
		if (width <= 0.0f || height <= 0.0f)
			return;

		aspectRatio = width / height;
	}

	glm::mat4 Camera::GetView() const
	{
		return glm::mat4_cast(glm::conjugate(rotation)) *
			glm::translate(glm::mat4(1.0f), -position);
	}

	glm::mat4 Camera::GetProjection() const
	{
		if (projectionMode == ProjectionMode::Perspective)
		{
			return glm::perspectiveRH_ZO(
				glm::radians(verticalFieldOfViewDegrees),
				aspectRatio,
				nearPlane,
				farPlane);
		}

		const float halfHeight = orthographicSize * 0.5f;
		const float halfWidth = halfHeight * aspectRatio;
		return glm::orthoRH_ZO(
			-halfWidth,
			halfWidth,
			-halfHeight,
			halfHeight,
			nearPlane,
			farPlane);
	}
}
