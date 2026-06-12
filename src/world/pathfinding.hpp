#pragma once

#include <limits>
#include <utility>
#include <engine/lib/grid2d.hpp>

// Passive pathfinding data types. These live in world/ (not systems/) so that map data can
// describe its path mesh and walkability without depending on the solver: world is the passive
// data layer, the Pathfinder algorithm that consumes these types lives in systems/.

// Per-cell result of a BFS solve: shortest hop distance to the goal and the next cell along
// that shortest path (the predecessor pointing back toward the goal).
struct Node {
    int m_distance = std::numeric_limits<int>::max();
    std::pair<int, int> m_predecessor = {-1, -1};
};

// Abstract walkability input: one cell per grid position, non-zero meaning walkable. Using an
// unsigned-char grid (rather than Grid2D<bool>, whose std::vector<bool> backing can't return a
// reference) keeps the solver free of any Tile/Map type.
using WalkableMask = Grid2D<unsigned char>;
