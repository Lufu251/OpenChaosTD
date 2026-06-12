#pragma once

#include <random>
#include <world/map.hpp>

// Tunable parameters for procedural map generation; loaded from map_generation.toml [map].
struct MapGenCfg {
    int minCluster       = 3;  // smallest rock cluster blob
    int maxCluster       = 7;  // largest rock cluster blob
    int seedTries        = 64; // attempts to find a free seed tile per cluster
    int growTries        = 16; // attempts to extend each cluster before giving up
    int tilesPerBuffTile = 44; // map area (tiles) allocated per buff tile placed
};

// Builds a playable Map: sizes the grid, places the core and spawn nests, grows
// random rock obstacles, and produces a ready-to-use path mesh. Each step is a
// discrete method so future layouts/strategies slot in without touching callers.
class MapGenerator {
public:
    // obstacleCount is a fixed target so every generated map of the same size keeps
    // the same number of buildable tiles (consistent difficulty); only the layout is random.
    void Generate(Map& map, int cols, int rows, int nestCount, int obstacleCount, const MapGenCfg& cfg = {});

private:
    void PlaceNests(Map& map, int nestCount);
    void PlaceObstacles(Map& map, int obstacleCount, const MapGenCfg& cfg);
    void PlaceBuffTiles(Map& map, int count, const MapGenCfg& cfg);
    void GrowCluster(Map& map, int& placed, int target, const MapGenCfg& cfg);
    bool TryPlaceRock(Map& map, int x, int y); // keep the rock only if all nests still reach core
    int  RandInt(int lo, int hi);

    std::mt19937 m_rng{std::random_device{}()};
};
