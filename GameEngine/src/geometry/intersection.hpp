#pragma once

#include <optional>

#include <glm/vec3.hpp>

namespace Ludus::Geometry
{
	struct Ray
	{
		glm::vec3 origin{ 0.0f };
		glm::vec3 direction{ 0.0f, 0.0f, -1.0f };
	};

	struct RayHit
	{
		float distance = 0.0f;
		glm::vec3 position{ 0.0f };
		glm::vec3 normal{ 0.0f };
	};

	struct Plane
	{
		glm::vec3 point{ 0.0f };
		glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
	};

	struct Aabb
	{
		glm::vec3 minimum{ 0.0f };
		glm::vec3 maximum{ 0.0f };
	};

	struct Sphere
	{
		glm::vec3 center{ 0.0f };
		float radius = 0.0f;
	};

	struct Triangle
	{
		glm::vec3 first{ 0.0f };
		glm::vec3 second{ 0.0f };
		glm::vec3 third{ 0.0f };
	};

	struct RaySegmentClosestPoints
	{
		float rayDistance = 0.0f;
		float segmentFraction = 0.0f;
		glm::vec3 rayPoint{ 0.0f };
		glm::vec3 segmentPoint{ 0.0f };
		float distanceSquared = 0.0f;
	};

	std::optional<RayHit> Intersect(const Ray& ray, const Plane& plane);
	std::optional<RayHit> Intersect(const Ray& ray, const Aabb& bounds);
	std::optional<RayHit> Intersect(const Ray& ray, const Sphere& sphere);
	std::optional<RayHit> Intersect(const Ray& ray, const Triangle& triangle);

	RaySegmentClosestPoints ClosestPoints(
		const Ray& ray,
		glm::vec3 segmentStart,
		glm::vec3 segmentEnd);
}
