#include <systems/map_generator.hpp>
#include <world/tile.hpp>
#include <algorithm>

namespace {
    constexpr int kMinCluster = 3;   // smallest rock blob
    constexpr int kMaxCluster = 7;   // largest rock blobs
    constexpr int kSeedTries  = 64;  // tries to find a free seed tile for a new cluster
    constexpr int kGrowTries  = 16;  // tries to extend a cluster before giving up

    constexpr int kTilesPerBuffTile = 44; // grid area per buff tile (density)

    // Round-robin set of terrain buffs; one is assigned per placed buff tile in order.
    struct BuffSpec { const char* m_statKey; float m_value; bool m_mul; };
    constexpr BuffSpec kBuffSpecs[] = {
        { "range",          30.0f, false }, // flat +range
        { "damage",          1.5f, true }, // mul *damage
        { "shotsPerMinute", 1.5f, true }, // mul *fire rate
    };
    constexpr int kBuffSpecCount = static_cast<int>(sizeof(kBuffSpecs) / sizeof(kBuffSpecs[0]));
}

void MapGenerator::Generate(Map& map, int cols, int rows, int nestCount, int obstacleCount){
    map.Create(cols, rows);

    // Core: centered on the bottom edge, one tile in from the border
    map.SetCore((cols - 1) / 2, rows - 2);

    PlaceNests(map, nestCount);
    PlaceObstacles(map, obstacleCount);

    // Range-buff terrain: count scales with map size. Buff tiles stay walkable, so they never
    // affect pathing and need no validation.
    PlaceBuffTiles(map, std::max(1, (cols * rows) / kTilesPerBuffTile));

    map.BuildPathMesh(); // final, clean path mesh for the chosen layout
}

void MapGenerator::PlaceNests(Map& map, int nestCount){
    int cols = map.GetCols();
    // Clamp so every nest fits along the top edge without overlapping
    nestCount = std::clamp(nestCount, 1, std::max(1, cols - 2));

    const int row = 1; // one tile in from the top border
    for (int i = 0; i < nestCount; i++) {
        // Evenly space across the width with a margin at both ends.
        // (i+1)*cols/(nestCount+1) centers a single nest and spreads many symmetrically.
        map.AddNest((i + 1) * cols / (nestCount + 1), row);
    }
}

void MapGenerator::PlaceObstacles(Map& map, int obstacleCount){
    // Grow clusters until the fixed obstacle target is met. A guard bounds the loop
    // in case the map is too small/saturated to ever reach the target.
    int placed = 0;
    int guard = 0;
    const int maxGuard = obstacleCount * 4 + 32;
    while (placed < obstacleCount && guard++ < maxGuard)
        GrowCluster(map, placed, obstacleCount);
}

void MapGenerator::PlaceBuffTiles(Map& map, int count){
    int cols = map.GetCols();
    int rows = map.GetRows();

    // Convert random open grass tiles into buff terrain, cycling through the buff specs so each
    // map gets an even mix of range/damage/speed tiles. Buff tiles stay walkable+buildable, so no
    // path validation is needed; only grass is eligible (core/nests/rocks are skipped).
    int placed = 0;
    while (placed < count) {
        int sx = -1, sy = -1;
        for (int t = 0; t < kSeedTries; t++) {
            int x = RandInt(0, cols - 1);
            int y = RandInt(0, rows - 1);
            if (map.Get(x, y).m_type == TileType::Grass) { sx = x; sy = y; break; }
        }
        if (sx < 0) break; // map too saturated to seed another buff tile

        const BuffSpec& spec = kBuffSpecs[placed % kBuffSpecCount];
        map.SetBuff(sx, sy, spec.m_statKey, spec.m_value, spec.m_mul);
        placed++;
    }
}

void MapGenerator::GrowCluster(Map& map, int& placed, int target){
    int cols = map.GetCols();
    int rows = map.GetRows();

    // Seed the cluster on a random free tile
    int sx = -1, sy = -1;
    for (int t = 0; t < kSeedTries; t++) {
        int x = RandInt(0, cols - 1);
        int y = RandInt(0, rows - 1);
        if (map.Get(x, y).m_type == TileType::Grass) { sx = x; sy = y; break; }
    }
    if (sx < 0 || !TryPlaceRock(map, sx, sy)) return;

    std::vector<std::pair<int, int>> cluster{ {sx, sy} };
    placed++;

    int clusterTarget = RandInt(kMinCluster, kMaxCluster);
    static const int dx[] = { 1, -1, 0, 0 };
    static const int dy[] = { 0, 0, 1, -1 };

    // Random-walk outward from tiles already in the cluster
    while (static_cast<int>(cluster.size()) < clusterTarget && placed < target) {
        bool extended = false;
        for (int t = 0; t < kGrowTries; t++) {
            auto [cx, cy] = cluster[RandInt(0, static_cast<int>(cluster.size()) - 1)];
            int d = RandInt(0, 3);
            int nx = cx + dx[d];
            int ny = cy + dy[d];

            if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) continue;
            if (map.Get(nx, ny).m_type != TileType::Grass) continue;

            if (TryPlaceRock(map, nx, ny)) {
                cluster.push_back({nx, ny});
                placed++;
                extended = true;
                break;
            }
            // rock rejected (would trap a nest) — leave it grass, try another neighbor
        }
        if (!extended) break; // cluster boxed in; let the next seed start elsewhere
    }
}

bool MapGenerator::TryPlaceRock(Map& map, int x, int y){
    Tile& tile = map.Get(x, y);
    tile.m_type = TileType::Rock;
    tile.m_walkable = false;
    tile.m_buildable = false;

    map.BuildPathMesh();
    if (map.ValidatePathMesh()) return true;

    // Reverting: this rock would cut a nest off from the core
    tile.m_type = TileType::Grass;
    tile.m_walkable = true;
    tile.m_buildable = true;
    return false;
}

int MapGenerator::RandInt(int lo, int hi){
    return std::uniform_int_distribution<int>(lo, hi)(m_rng);
}
