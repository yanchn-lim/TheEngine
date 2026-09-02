#include "intersection.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include <glm/geometric.hpp>

namespace Ludus::Geometry
{
	namespace
	{
		constexpr float Epsilon = 1.0e-6f;

		bool HasDirection(const Ray& ray)
		{
			return glm::dot(ray.direction, ray.direction) > Epsilon * Epsilon;
		}

		RayHit MakeHit(
			const Ray& ray,
			float distance,
			glm::vec3 normal)
		{
			return {
				distance,
				ray.origin + ray.direction * distance,
				normal
			};
		}
	}

	std::optional<RayHit> Intersect(const Ray& ray, const Plane& plane)
	{
		if (!HasDirection(ray))
			return std::nullopt;

		const float normalLengthSquared = glm::dot(plane.normal, plane.normal);
		if (normalLengthSquared <= Epsilon * Epsilon)
			return std::nullopt;

		const glm::vec3 normal = plane.normal / std::sqrt(normalLengthSquared);
		const float denominator = glm::dot(ray.direction, normal);
		if (std::abs(denominator) <= Epsilon)
			return std::nullopt;

		const float distance =
			glm::dot(plane.point - ray.origin, normal) / denominator;
		if (distance < 0.0f)
			return std::nullopt;

		return MakeHit(
			ray,
			distance,
			denominator < 0.0f ? normal : -normal);
	}

	std::optional<RayHit> Intersect(const Ray& ray, const Aabb& bounds)
	{
		if (!HasDirection(ray) ||
			bounds.minimum.x > bounds.maximum.x ||
			bounds.minimum.y > bounds.maximum.y ||
			bounds.minimum.z > bounds.maximum.z)
		{
			return std::nullopt;
		}

		float minimumDistance = 0.0f;
		float maximumDistance = std::numeric_limits<float>::max();
		glm::vec3 minimumNormal{ 0.0f };
		glm::vec3 maximumNormal{ 0.0f };

		for (int axis = 0; axis < 3; ++axis)
		{
			if (std::abs(ray.direction[axis]) <= Epsilon)
			{
				if (ray.origin[axis] < bounds.minimum[axis] ||
					ray.origin[axis] > bounds.maximum[axis])
				{
					return std::nullopt;
				}
				continue;
			}

			float first =
				(bounds.minimum[axis] - ray.origin[axis]) / ray.direction[axis];
			float second =
				(bounds.maximum[axis] - ray.origin[axis]) / ray.direction[axis];
			glm::vec3 firstNormal{ 0.0f };
			glm::vec3 secondNormal{ 0.0f };
			firstNormal[axis] = -1.0f;
			secondNormal[axis] = 1.0f;

			if (first > second)
			{
				std::swap(first, second);
				std::swap(firstNormal, secondNormal);
			}
			if (first > minimumDistance)
			{
				minimumDistance = first;
				minimumNormal = firstNormal;
			}
			if (second < maximumDistance)
			{
				maximumDistance = second;
				maximumNormal = secondNormal;
			}
			if (minimumDistance > maximumDistance)
				return std::nullopt;
		}

		if (minimumDistance > 0.0f)
			return MakeHit(ray, minimumDistance, minimumNormal);
		return MakeHit(ray, maximumDistance, maximumNormal);
	}

	std::optional<RayHit> Intersect(const Ray& ray, const Sphere& sphere)
	{
		if (!HasDirection(ray) || sphere.radius < 0.0f)
			return std::nullopt;

		const float directionLengthSquared =
			glm::dot(ray.direction, ray.direction);
		const glm::vec3 offset = ray.origin - sphere.center;
		const float halfB = glm::dot(offset, ray.direction);
		const float c = glm::dot(offset, offset) - sphere.radius * sphere.radius;
		const float discriminant =
			halfB * halfB - directionLengthSquared * c;
		if (discriminant < 0.0f)
			return std::nullopt;

		const float root = std::sqrt(discriminant);
		float distance =
			(-halfB - root) / directionLengthSquared;
		if (distance < 0.0f)
			distance = (-halfB + root) / directionLengthSquared;
		if (distance < 0.0f)
			return std::nullopt;

		const glm::vec3 position =
			ray.origin + ray.direction * distance;
		glm::vec3 normal{ 0.0f };
		if (sphere.radius > Epsilon)
			normal = (position - sphere.center) / sphere.radius;
		return RayHit{ distance, position, normal };
	}

	std::optional<RayHit> Intersect(const Ray& ray, const Triangle& triangle)
	{
		if (!HasDirection(ray))
			return std::nullopt;

		const glm::vec3 firstEdge = triangle.second - triangle.first;
		const glm::vec3 secondEdge = triangle.third - triangle.first;
		const glm::vec3 cross = glm::cross(ray.direction, secondEdge);
		const float determinant = glm::dot(firstEdge, cross);
		if (std::abs(determinant) <= Epsilon)
			return std::nullopt;

		const float inverseDeterminant = 1.0f / determinant;
		const glm::vec3 offset = ray.origin - triangle.first;
		const float firstWeight =
			glm::dot(offset, cross) * inverseDeterminant;
		if (firstWeight < 0.0f || firstWeight > 1.0f)
			return std::nullopt;

		const glm::vec3 directionCross = glm::cross(offset, firstEdge);
		const float secondWeight =
			glm::dot(ray.direction, directionCross) * inverseDeterminant;
		if (secondWeight < 0.0f || firstWeight + secondWeight > 1.0f)
			return std::nullopt;

		const float distance =
			glm::dot(secondEdge, directionCross) * inverseDeterminant;
		if (distance < 0.0f)
			return std::nullopt;

		glm::vec3 normal = glm::normalize(glm::cross(firstEdge, secondEdge));
		if (glm::dot(normal, ray.direction) > 0.0f)
			normal = -normal;
		return MakeHit(ray, distance, normal);
	}

	RaySegmentClosestPoints ClosestPoints(
		const Ray& ray,
		glm::vec3 segmentStart,
		glm::vec3 segmentEnd)
	{
		const glm::vec3 segment = segmentEnd - segmentStart;
		const glm::vec3 offset = ray.origin - segmentStart;
		const float rayLengthSquared = glm::dot(ray.direction, ray.direction);
		const float segmentLengthSquared = glm::dot(segment, segment);
		const float crossLength = glm::dot(ray.direction, segment);
		const float rayOffset = glm::dot(ray.direction, offset);
		const float segmentOffset = glm::dot(segment, offset);

		float rayDistance = 0.0f;
		float segmentFraction = 0.0f;
		const float denominator =
			rayLengthSquared * segmentLengthSquared -
			crossLength * crossLength;

		if (rayLengthSquared > Epsilon * Epsilon &&
			segmentLengthSquared > Epsilon * Epsilon &&
			std::abs(denominator) > Epsilon)
		{
			rayDistance = std::max(
				0.0f,
				(crossLength * segmentOffset -
					segmentLengthSquared * rayOffset) /
					denominator);
			segmentFraction = std::clamp(
				(rayLengthSquared * segmentOffset -
					crossLength * rayOffset) /
					denominator,
				0.0f,
				1.0f);
			rayDistance = std::max(
				0.0f,
				(crossLength * segmentFraction - rayOffset) /
					rayLengthSquared);
		}
		else if (segmentLengthSquared > Epsilon * Epsilon)
		{
			segmentFraction = std::clamp(
				segmentOffset / segmentLengthSquared,
				0.0f,
				1.0f);
		}

		const glm::vec3 rayPoint =
			ray.origin + ray.direction * rayDistance;
		const glm::vec3 segmentPoint =
			segmentStart + segment * segmentFraction;
		const glm::vec3 difference = rayPoint - segmentPoint;
		return {
			rayDistance,
			segmentFraction,
			rayPoint,
			segmentPoint,
			glm::dot(difference, difference)
		};
	}
}
