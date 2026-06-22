#include "transform2d.hpp"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Transform2D::GetMatrix() const
{
	glm::mat4 matrix{ 1.f };

	matrix = glm::translate(matrix, glm::vec3(position, 0.f));
	matrix = glm::rotate(matrix, rotation, glm::vec3(0.f, 0.f, 1.f));
	matrix = glm::scale(matrix, glm::vec3(scale, 1.f));

	return matrix;
}
