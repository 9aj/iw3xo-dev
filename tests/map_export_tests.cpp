#include "../src/utils/map_export.hpp"
#include <cassert>
#include <iostream>

// Same leaf/children union layout used by IW3, without the game runtime.
struct Node
{
	short leafBrushCount = 0;
	union Data
	{
		struct { unsigned short* brushes; } leaf;
		struct { float dist, range; unsigned short childOffset[2]; } children;
		Data() : children{} {}
	} data;
};

int main()
{
	using utils::map_export::collect_brush_indices;
	Node nodes[8];
	unsigned short first[] = { 4, 2, 4, 99 };
	nodes[1].leafBrushCount = 4;
	nodes[1].data.leaf.brushes = first;
	assert((collect_brush_indices(nodes, 8, 1, 10) == std::vector<unsigned short>{ 2, 4 }));

	// Split submodel: both children, every brush, overlapping indices deduplicated.
	unsigned short second[] = { 1, 2 };
	nodes[2].data.children.childOffset[0] = 1;
	nodes[2].data.children.childOffset[1] = 2;
	nodes[3] = nodes[1];
	nodes[4].leafBrushCount = 2;
	nodes[4].data.leaf.brushes = second;
	assert((collect_brush_indices(nodes, 8, 2, 10) == std::vector<unsigned short>{ 1, 2, 4 }));

	// Negative-count internal node has a third, overlapping-brush subtree at +1.
	unsigned short overlap[] = { 7 };
	nodes[2].leafBrushCount = -1;
	nodes[2].data.children.childOffset[0] = 2;
	nodes[2].data.children.childOffset[1] = 3;
	nodes[5].leafBrushCount = 1;
	nodes[5].data.leaf.brushes = overlap;
	assert((collect_brush_indices(nodes, 8, 2, 10) == std::vector<unsigned short>{ 1, 2, 4, 7 }));

	// Invalid roots, null leaves, empty nodes and bad offsets are bounded.
	assert(collect_brush_indices<Node>(nullptr, 8, 1, 10).empty());
	assert(collect_brush_indices(nodes, 8, 0, 10).empty());
	assert(collect_brush_indices(nodes, 8, -1, 10).empty());
	assert(collect_brush_indices(nodes, 8, 8, 10).empty());
	nodes[6].leafBrushCount = 1;
	nodes[6].data.leaf.brushes = nullptr;
	assert(collect_brush_indices(nodes, 8, 6, 10).empty());
	nodes[7].data.children.childOffset[0] = 60000;
	assert(collect_brush_indices(nodes, 8, 7, 10).empty());
	assert(collect_brush_indices(nodes, 8, 1, 0).empty());
	std::cout << "Map-export traversal regression tests passed.\n";
}
