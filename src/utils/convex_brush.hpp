#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <utility>
#include <vector>

namespace utils::map_export
{
	using point = std::array<double, 3>;
	struct brush_plane { point normal; double distance; };
	struct brush_face
	{
		// Original axial side [0,6), or additional brush side + 6.
		unsigned int side_index;
		std::array<point, 3> points;
	};
	inline std::string format_plane_points(const brush_face& face)
	{
		std::ostringstream output;
		output.imbue(std::locale::classic());
		output << std::fixed << std::setprecision(9);
		for (const auto& p : face.points) output << " ( " << p[0] << ' ' << p[1] << ' ' << p[2] << " )";
		output << ' ';
		return output.str();
	}

	inline point subtract(const point& a, const point& b)
	{
		return { a[0] - b[0], a[1] - b[1], a[2] - b[2] };
	}
	inline double dot(const point& a, const point& b)
	{
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	}
	inline point cross(const point& a, const point& b)
	{
		return { a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };
	}
	inline bool same_point(const point& a, const point& b)
	{
		const auto delta = subtract(a, b);
		return dot(delta, delta) < 1e-12;
	}

	// Clip the bounding box by the collision planes. Export is independent of
	// debug winding limits, snapping, camera culling and plane-intersection storage.
	inline std::vector<brush_face> reconstruct_brush(const point& mins, const point& maxs,
		const std::vector<brush_plane>& additional_planes)
	{
		constexpr double epsilon = 1e-6;
		for (int axis = 0; axis < 3; ++axis)
		{
			if (!std::isfinite(mins[axis]) || !std::isfinite(maxs[axis]) || mins[axis] >= maxs[axis]) return {};
		}
		struct polygon { unsigned int side_index; point normal; std::vector<point> points; };
		std::array<point, 8> corners;
		for (int mask = 0; mask < 8; ++mask)
		{
			for (int axis = 0; axis < 3; ++axis) corners[mask][axis] = mask & (1 << axis) ? maxs[axis] : mins[axis];
		}
		const int indices[6][4] = { {0,4,6,2}, {1,3,7,5}, {0,1,5,4}, {2,6,7,3}, {0,2,3,1}, {4,5,7,6} };
		std::vector<polygon> polygons;
		for (unsigned int side = 0; side < 6; ++side)
		{
			polygon face{ side, {}, {} };
			face.normal[side / 2] = side % 2 ? 1.0 : -1.0;
			for (const int index : indices[side]) face.points.push_back(corners[index]);
			polygons.push_back(std::move(face));
		}
		for (unsigned int side = 0; side < additional_planes.size(); ++side)
		{
			auto plane = additional_planes[side];
			const double length = std::sqrt(dot(plane.normal, plane.normal));
			if (!std::isfinite(length) || length < 1e-12 || !std::isfinite(plane.distance)) return {};
			for (auto& component : plane.normal) component /= length;
			plane.distance /= length;
			std::vector<polygon> clipped;
			std::vector<point> cap;
			for (const auto& face : polygons)
			{
				polygon next{ face.side_index, face.normal, {} };
				for (size_t i = 0; i < face.points.size(); ++i)
				{
					const auto& a = face.points[i];
					const auto& b = face.points[(i + 1) % face.points.size()];
					double da = dot(plane.normal, a) - plane.distance;
					double db = dot(plane.normal, b) - plane.distance;
					if (std::abs(da) <= epsilon) da = 0;
					if (std::abs(db) <= epsilon) db = 0;
					if (da <= 0) next.points.push_back(a);
					if ((da <= 0) != (db <= 0))
					{
						const double fraction = da / (da - db);
						point intersection;
						for (int axis = 0; axis < 3; ++axis) intersection[axis] = a[axis] + fraction * (b[axis] - a[axis]);
						next.points.push_back(intersection);
						if (std::none_of(cap.begin(), cap.end(), [&](const point& p) { return same_point(p, intersection); })) cap.push_back(intersection);
					}
				}
				next.points.erase(std::unique(next.points.begin(), next.points.end(), same_point), next.points.end());
				if (next.points.size() > 1 && same_point(next.points.front(), next.points.back())) next.points.pop_back();
				if (next.points.size() >= 3) clipped.push_back(std::move(next));
			}
			if (cap.size() >= 3)
			{
				point center{};
				for (const auto& p : cap) for (int axis = 0; axis < 3; ++axis) center[axis] += p[axis] / cap.size();
				const auto u = cross(plane.normal, std::abs(plane.normal[0]) < 0.9 ? point{1,0,0} : point{0,1,0});
				const auto v = cross(plane.normal, u);
				std::sort(cap.begin(), cap.end(), [&](const point& a, const point& b)
				{
					const auto da = subtract(a, center), db = subtract(b, center);
					return std::atan2(dot(da, v), dot(da, u)) < std::atan2(dot(db, v), dot(db, u));
				});
				clipped.push_back({ side + 6, plane.normal, std::move(cap) });
			}
			polygons = std::move(clipped);
			if (polygons.empty()) return {};
		}

		std::vector<brush_face> result;
		double volume = 0;
		for (const auto& polygon : polygons)
		{
			brush_face face{ polygon.side_index, {} };
			double best_area = 0;
			for (size_t i = 0; i < polygon.points.size(); ++i)
			{
				for (size_t j = i + 1; j < polygon.points.size(); ++j)
				{
					for (size_t k = j + 1; k < polygon.points.size(); ++k)
					{
						const auto normal = cross(subtract(polygon.points[j], polygon.points[i]), subtract(polygon.points[k], polygon.points[i]));
						const double area = std::abs(dot(normal, polygon.normal));
						if (area > best_area)
						{
							best_area = area;
							face.points = { polygon.points[i], polygon.points[j], polygon.points[k] };
							// Radiant uses cross(p2-p0, p1-p0) for its outward plane.
							if (dot(normal, polygon.normal) > 0) std::swap(face.points[1], face.points[2]);
						}
					}
				}
			}
			if (best_area <= 1e-10) continue;
			for (size_t j = 1; j + 1 < polygon.points.size(); ++j)
			{
				const auto a = subtract(polygon.points[0], mins);
				const auto b = subtract(polygon.points[j], mins);
				const auto c = subtract(polygon.points[j + 1], mins);
				volume += dot(a, cross(b, c)) / 6;
			}
			result.push_back(face);
		}
		if (result.size() < 4 || !std::isfinite(volume) || volume <= 1e-10) return {};
		return result;
	}
}
