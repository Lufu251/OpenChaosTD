#pragma once

#include <utility>
#include <engine/lib/grid2d.hpp>
#include <world/pathfinding.hpp>

// Breadth-first solver over an abstract walkable grid. It knows nothing about tiles or maps:
// it consumes a walkability mask plus a goal cell and returns the per-cell distance/predecessor
// mesh that flows back toward the goal. The Node/WalkableMask data types live in world/pathfinding.hpp.
class Pathfinder {
public:
    static Grid2D<Node> Solve(const WalkableMask& walkable, std::pair<int, int> goal);
};
