#include <glm/gtc/matrix_transform.hpp>

#include "camera.hpp"

namespace Graphics
{
	void Camera2D::SetViewport(float width, float height)
	{
		if (height <= 0.f)
			return;

		aspectRatio = width / height;
	}

	glm::mat4 Camera2D::GetView() const
	{
		return
			glm::rotate(glm::mat4(1.f), -rotation, glm::vec3(0, 0, 1)) *
			glm::translate(glm::mat4(1.f), glm::vec3(-position,0.f));
	}

	glm::mat4 Camera2D::GetProjection() const
	{
		return glm::ortho
		(
			-aspectRatio * zoom,
			aspectRatio * zoom,
			-zoom,
			zoom,
			-1.f,
			1.f
		);
	}
}
