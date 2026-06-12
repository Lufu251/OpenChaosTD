#pragma once

#include <random>
#include <world/map.hpp>

// Builds a playable Map: sizes the grid, places the core and spawn nests, grows
// random rock obstacles, and produces a ready-to-use path mesh. Each step is a
// discrete method so future layouts/strategies slot in without touching callers.
class MapGenerator {
public:
    // obstacleCount is a fixed target so every generated map of the same size keeps
    // the same number of buildable tiles (consistent difficulty); only the layout is random.
    void Generate(Map& map, int cols, int rows, int nestCount, int obstacleCount);

private:
    void PlaceNests(Map& map, int nestCount);
    void PlaceObstacles(Map& map, int obstacleCount);
    void PlaceBuffTiles(Map& map, int count); // seed range-buff terrain on open grass
    void GrowCluster(Map& map, int& placed, int target);
    bool TryPlaceRock(Map& map, int x, int y); // keep the rock only if all nests still reach core
    int  RandInt(int lo, int hi);

    std::mt19937 m_rng{std::random_device{}()};
};
