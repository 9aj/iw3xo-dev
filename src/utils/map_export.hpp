#pragma once

#include <algorithm>
#include <vector>

namespace utils::map_export
{
	// IW3 leaf nodes contain either a brush list or relative child offsets.
	// Negative counts also have an overlapping-brush subtree at node + 1.
	template <typename Node>
	std::vector<unsigned short> collect_brush_indices(const Node* nodes, int node_count,
		int root, unsigned int brush_count)
	{
		std::vector<unsigned short> brushes;
		if (!nodes || root <= 0 || root >= node_count)
		{
			return brushes;
		}

		std::vector<int> pending{ root };
		std::vector<bool> visited(node_count, false);
		while (!pending.empty())
		{
			const int index = pending.back();
			pending.pop_back();
			if (index <= 0 || index >= node_count || visited[index])
			{
				continue;
			}
			visited[index] = true;
			const auto& node = nodes[index];
			if (node.leafBrushCount > 0)
			{
				if (!node.data.leaf.brushes) continue;
				for (int i = 0; i < node.leafBrushCount; ++i)
				{
					const auto brush = node.data.leaf.brushes[i];
					if (brush < brush_count) brushes.push_back(brush);
				}
			}
			else
			{
				if (node.leafBrushCount < 0) pending.push_back(index + 1);
				for (const auto offset : node.data.children.childOffset)
				{
					if (offset) pending.push_back(index + offset);
				}
			}
		}
		std::sort(brushes.begin(), brushes.end());
		brushes.erase(std::unique(brushes.begin(), brushes.end()), brushes.end());
		return brushes;
	}
}
