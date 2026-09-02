#include "geometry_tests.hpp"

#include <cmath>

#include "geometry/intersection.hpp"

namespace Tests
{
	namespace
	{
		constexpr float Epsilon = 1.0e-4f;

		bool Near(float left, float right)
		{
			return std::abs(left - right) <= Epsilon;
		}

		bool TestPlane()
		{
			const Ludus::Geometry::Ray ray{
				glm::vec3(0.0f, 2.0f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f) };
			const Ludus::Geometry::Plane plane{
				glm::vec3(0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f) };
			const auto hit = Ludus::Geometry::Intersect(
				ray, plane);
			return hit && Near(hit->distance, 2.0f) &&
				Near(hit->position.y, 0.0f) &&
				!Ludus::Geometry::Intersect(
					Ludus::Geometry::Ray{
						glm::vec3(0.0f, 2.0f, 0.0f),
						glm::vec3(1.0f, 0.0f, 0.0f) },
					plane);
		}

		bool TestAabb()
		{
			const Ludus::Geometry::Aabb bounds{
				{ -1.0f, -1.0f, -1.0f },
				{ 1.0f, 1.0f, 1.0f }
			};
			const auto outside = Ludus::Geometry::Intersect(
				Ludus::Geometry::Ray{
					glm::vec3(0.0f, 0.0f, 3.0f),
					glm::vec3(0.0f, 0.0f, -1.0f) },
				bounds);
			const auto inside = Ludus::Geometry::Intersect(
				Ludus::Geometry::Ray{
					glm::vec3(0.0f),
					glm::vec3(1.0f, 0.0f, 0.0f) },
				bounds);
			return outside && Near(outside->distance, 2.0f) &&
				Near(outside->normal.z, 1.0f) &&
				inside && Near(inside->distance, 1.0f) &&
				Near(inside->normal.x, 1.0f);
		}

		bool TestSphere()
		{
			const auto hit = Ludus::Geometry::Intersect(
				Ludus::Geometry::Ray{
					glm::vec3(0.0f, 0.0f, 3.0f),
					glm::vec3(0.0f, 0.0f, -1.0f) },
				Ludus::Geometry::Sphere{ glm::vec3(0.0f), 1.0f });
			return hit && Near(hit->distance, 2.0f) &&
				Near(hit->normal.z, 1.0f);
		}

		bool TestTriangle()
		{
			const Ludus::Geometry::Triangle triangle{
				{ -1.0f, -1.0f, 0.0f },
				{ 1.0f, -1.0f, 0.0f },
				{ 0.0f, 1.0f, 0.0f }
			};
			const auto hit = Ludus::Geometry::Intersect(
				Ludus::Geometry::Ray{
					glm::vec3(0.0f, 0.0f, 1.0f),
					glm::vec3(0.0f, 0.0f, -1.0f) },
				triangle);
			return hit && Near(hit->distance, 1.0f) &&
				!Ludus::Geometry::Intersect(
					Ludus::Geometry::Ray{
						glm::vec3(2.0f, 0.0f, 1.0f),
						glm::vec3(0.0f, 0.0f, -1.0f) },
					triangle);
		}

		bool TestClosestPoints()
		{
			const auto result = Ludus::Geometry::ClosestPoints(
				Ludus::Geometry::Ray{
					glm::vec3(0.0f, 1.0f, 0.0f),
					glm::vec3(1.0f, 0.0f, 0.0f) },
				glm::vec3(2.0f, 0.0f, -1.0f),
				glm::vec3(2.0f, 0.0f, 1.0f));
			return Near(result.rayDistance, 2.0f) &&
				Near(result.segmentFraction, 0.5f) &&
				Near(result.distanceSquared, 1.0f);
		}
	}

	bool RunGeometryTests()
	{
		return TestPlane() &&
			TestAabb() &&
			TestSphere() &&
			TestTriangle() &&
			TestClosestPoints();
	}
}
