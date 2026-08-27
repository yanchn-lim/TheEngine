#include <glm/gtc/matrix_transform.hpp>

#include "camera.hpp"

namespace Ludus::Graphics
{
	void Camera2D::SetViewport(float width, float height)
	{
		// store aspect ratio so the orthographic projection preserves screen shape
		if (height <= 0.f)
			return;

		aspectRatio = width / height;
	}

	glm::mat4 Camera2D::GetView() const
	{
		// camera movement is inverted to transform world coordinates into view space
		return
			glm::rotate(glm::mat4(1.f), -rotation, glm::vec3(0, 0, 1)) *
			glm::translate(glm::mat4(1.f), glm::vec3(-position,0.f));
	}

	glm::mat4 Camera2D::GetProjection() const
	{
		// zoom controls vertical half-size while aspect ratio expands horizontal range
		return glm::orthoRH_ZO
		(
			-aspectRatio * zoom,
			aspectRatio * zoom,
			-zoom,
			zoom,
			-10.f,
			10.f
		);
	}
}
