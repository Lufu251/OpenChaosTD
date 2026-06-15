#pragma once

#include <random>
#include <string>
#include <vector>
#include <world/map.hpp>

class TileFactory;

// Tunable parameters for procedural map generation; loaded from map_generation.toml [map].
struct MapGenCfg {
    int minCluster       = 3;  // smallest rock cluster blob
    int maxCluster       = 7;  // largest rock cluster blob
    int seedTries        = 64; // attempts to find a free seed tile per cluster
    int growTries        = 16; // attempts to extend each cluster before giving up
    int tilesPerBuffTile = 44; // map area (tiles) allocated per buff tile placed
};

// Builds a playable Map: sizes the grid, places the core and spawn nests, grows
// random rock obstacles, and produces a ready-to-use path mesh. Tile IDs are
// resolved from the TileFactory by category, so any datapack that defines tiles
// with the right categories (goal/spawn/obstacle/buff/ground) is compatible.
class MapGenerator {
public:
    // obstacleCount is a fixed target so every generated map of the same size keeps
    // the same number of buildable tiles (consistent difficulty); only the layout is random.
    void Generate(Map& map, const TileFactory& tileFactory, int cols, int rows,
                  int nestCount, int obstacleCount, const MapGenCfg& cfg = {});

private:
    void PlaceNests(Map& map, const TileFactory& tileFactory, int nestCount);
    void PlaceObstacles(Map& map, const TileFactory& tileFactory, int obstacleCount, const MapGenCfg& cfg);
    void PlaceBuffTiles(Map& map, const TileFactory& tileFactory, int count, const MapGenCfg& cfg);
    void GrowCluster(Map& map, const TileFactory& tileFactory, int& placed, int target, const MapGenCfg& cfg);
    bool TryPlaceRock(Map& map, const TileFactory& tileFactory, int x, int y);
    int  RandInt(int lo, int hi);

    std::mt19937 m_rng{std::random_device{}()};

    // Resolved from the TileFactory on Generate; cached for helper access.
    std::string m_groundId;
    std::string m_obstacleId;
    std::vector<std::string> m_buffIds;
};
