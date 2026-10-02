#include "../src/utils/convex_brush.hpp"
#include <cassert>
#include <iostream>
#include <limits>

using namespace utils::map_export;

static void validate(const std::vector<brush_face>& faces, const point& mins, const point& maxs,
	const std::vector<brush_plane>& planes)
{
	assert(faces.size() >= 4);
	for (const auto& face : faces)
	{
		point expected{};
		if (face.side_index < 6) expected[face.side_index / 2] = face.side_index % 2 ? 1 : -1;
		else expected = planes.at(face.side_index - 6).normal;
		const auto normal = cross(subtract(face.points[2], face.points[0]), subtract(face.points[1], face.points[0]));
		assert(dot(normal, expected) > 1e-10);
		for (const auto& p : face.points)
		{
			for (int axis = 0; axis < 3; ++axis) assert(p[axis] >= mins[axis] - 2e-6 && p[axis] <= maxs[axis] + 2e-6);
			for (const auto& plane : planes) assert(dot(plane.normal, p) <= plane.distance + 2e-6);
		}
		// Parse the actual production plane formatter and check the plane survives.
		auto serialized = format_plane_points(face);
		std::replace(serialized.begin(), serialized.end(), '(', ' ');
		std::replace(serialized.begin(), serialized.end(), ')', ' ');
		std::istringstream input(serialized);
		for (const auto& p : face.points)
		{
			for (const double coordinate : p)
			{
				double parsed = 0;
				assert(static_cast<bool>(input >> parsed));
				assert(std::abs(parsed - coordinate) < 1e-8);
			}
		}
	}
}

int main()
{
	const point lo{0,0,0}, hi{1,1,1};
	auto box = reconstruct_brush(lo, hi, {});
	assert(box.size() == 6);
	validate(box, lo, hi, {});

	// Tetrahedral bounce brush: four valid faces must not be rejected.
	std::vector<brush_plane> tetra{ {{1,1,1}, 1} };
	auto faces = reconstruct_brush(lo, hi, tetra);
	assert(faces.size() == 4);
	validate(faces, lo, hi, tetra);

	// Triangular-prism ramp: two axial faces vanish, while the slope retains side 6.
	std::vector<brush_plane> ramp{ {{1,0,1}, 1} };
	faces = reconstruct_brush(lo, hi, ramp);
	assert(faces.size() == 5);
	assert(std::any_of(faces.begin(), faces.end(), [](const brush_face& f) { return f.side_index == 6; }));
	assert(std::none_of(faces.begin(), faces.end(), [](const brush_face& f) { return f.side_index == 1 || f.side_index == 5; }));
	validate(faces, lo, hi, ramp);

	// Fractional sloped brush near large/negative map coordinates.
	const point translated_lo{-12000.375, 16000.125, 2048.25};
	const point translated_hi{-11968.125, 16032.375, 2080.5};
	std::vector<brush_plane> translated{ {{1,0,1}, translated_lo[0] + translated_hi[2]} };
	faces = reconstruct_brush(translated_lo, translated_hi, translated);
	assert(faces.size() == 5);
	validate(faces, translated_lo, translated_hi, translated);
	assert(format_plane_points(faces[0]).find(".375000000") != std::string::npos);

	// Bevels and duplicate/redundant planes must not duplicate a brush face.
	std::vector<brush_plane> bevels{ {{1,1,0}, 1.5}, {{2,2,0}, 3}, {{1,0,0}, 2} };
	faces = reconstruct_brush(lo, hi, bevels);
	assert(faces.size() == 7);
	validate(faces, lo, hi, bevels);

	// No fixed 128-intersection limit: a 160-sided prism has 162 real faces.
	std::vector<brush_plane> prism;
	constexpr double pi = 3.14159265358979323846;
	for (int side = 0; side < 160; ++side)
	{
		const double angle = (side + 0.5) * 2 * pi / 160;
		prism.push_back({ {std::cos(angle), std::sin(angle), 0}, 64 });
	}
	const point prism_lo{-65,-65,-8}, prism_hi{65,65,8};
	faces = reconstruct_brush(prism_lo, prism_hi, prism);
	assert(faces.size() == 162);
	validate(faces, prism_lo, prism_hi, prism);

	assert(reconstruct_brush(lo, hi, {{{1,0,0}, -1}}).empty());
	assert(reconstruct_brush(lo, hi, {{{1,0,0}, 0}}).empty());
	assert(reconstruct_brush(lo, hi, {{{0,0,0}, 1}}).empty());
	assert(reconstruct_brush(lo, hi, {{{1,0,0}, std::numeric_limits<double>::quiet_NaN()}}).empty());
	assert(reconstruct_brush(hi, lo, {}).empty());
	std::cout << "Convex bounce-brush and plane-serialization regression tests passed.\n";
}
