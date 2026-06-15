#include <systems/map_generator.hpp>
#include <content/tile_factory.hpp>
#include <world/tile.hpp>
#include <algorithm>
#include <iostream>

void MapGenerator::Generate(Map& map, const TileFactory& tileFactory,
                            int cols, int rows, int nestCount, int obstacleCount,
                            const MapGenCfg& cfg){
    m_buffIds = tileFactory.GetBuffIds();

    if (!tileFactory.Has(cfg.groundId) || !tileFactory.Has(cfg.obstacleId)
        || !tileFactory.Has(cfg.coreId) || !tileFactory.Has(cfg.nestId)) {
        std::cerr << "MapGenerator: missing required tile IDs ("
                  << cfg.groundId << "/" << cfg.obstacleId << "/"
                  << cfg.coreId << "/" << cfg.nestId << ")\n";
        return;
    }

    map.Create(cols, rows, tileFactory, cfg.groundId);

    // Core: centered on the bottom edge, one tile in from the border
    map.ApplyTileDef((cols - 1) / 2, rows - 2, tileFactory, cfg.coreId);
    map.SetCore((cols - 1) / 2, rows - 2);

    PlaceNests(map, tileFactory, nestCount, cfg);
    PlaceObstacles(map, tileFactory, obstacleCount, cfg);

    // Buff terrain: count scales with map size. Buff tiles stay walkable, so they never
    // affect pathing and need no validation.
    PlaceBuffTiles(map, tileFactory, std::max(1, (cols * rows) / cfg.tilesPerBuffTile), cfg);

    map.BuildPathMesh(); // final, clean path mesh for the chosen layout
}

void MapGenerator::PlaceNests(Map& map, const TileFactory& tileFactory,
                               int nestCount, const MapGenCfg& cfg){
    int cols = map.GetCols();
    // Clamp so every nest fits along the top edge without overlapping
    nestCount = std::clamp(nestCount, 1, std::max(1, cols - 2));

    const int row = 1; // one tile in from the top border
    for (int i = 0; i < nestCount; i++) {
        // Evenly space across the width with a margin at both ends.
        // (i+1)*cols/(nestCount+1) centers a single nest and spreads many symmetrically.
        int nx = (i + 1) * cols / (nestCount + 1);
        map.ApplyTileDef(nx, row, tileFactory, cfg.nestId);
        map.AddNest(nx, row);
    }
}

void MapGenerator::PlaceObstacles(Map& map, const TileFactory& tileFactory,
                                   int obstacleCount, const MapGenCfg& cfg){
    // Grow clusters until the fixed obstacle target is met. A guard bounds the loop
    // in case the map is too small/saturated to ever reach the target.
    int placed = 0;
    int guard = 0;
    const int maxGuard = obstacleCount * 4 + 32;
    while (placed < obstacleCount && guard++ < maxGuard)
        GrowCluster(map, tileFactory, placed, obstacleCount, cfg);
}

void MapGenerator::PlaceBuffTiles(Map& map, const TileFactory& tileFactory,
                                   int count, const MapGenCfg& cfg){
    int cols = map.GetCols();
    int rows = map.GetRows();

    if (m_buffIds.empty()) return;

    // Convert random open ground tiles into buff terrain, cycling through the buff IDs so each
    // map gets an even mix of range/damage/speed tiles. Buff tiles stay walkable+buildable, so no
    // path validation is needed; only ground tiles are eligible (goal/spawn/obstacle are skipped).
    int placed = 0;
    while (placed < count) {
        int sx = -1, sy = -1;
        for (int t = 0; t < cfg.seedTries; t++) {
            int x = RandInt(0, cols - 1);
            int y = RandInt(0, rows - 1);
            if (map.Get(x, y).m_tileId == cfg.groundId) { sx = x; sy = y; break; }
        }
        if (sx < 0) break; // map too saturated to seed another buff tile

        const std::string& buffId = m_buffIds[placed % m_buffIds.size()];
        map.SetBuff(sx, sy, tileFactory, buffId);
        placed++;
    }
}

void MapGenerator::GrowCluster(Map& map, const TileFactory& tileFactory,
                                int& placed, int target, const MapGenCfg& cfg){
    int cols = map.GetCols();
    int rows = map.GetRows();

    // Seed the cluster on a random free ground tile
    int sx = -1, sy = -1;
    for (int t = 0; t < cfg.seedTries; t++) {
        int x = RandInt(0, cols - 1);
        int y = RandInt(0, rows - 1);
        if (map.Get(x, y).m_tileId == cfg.groundId) { sx = x; sy = y; break; }
    }
    if (sx < 0 || !TryPlaceRock(map, tileFactory, sx, sy, cfg)) return;

    std::vector<std::pair<int, int>> cluster{ {sx, sy} };
    placed++;

    int clusterTarget = RandInt(cfg.minCluster, cfg.maxCluster);
    static const int dx[] = { 1, -1, 0, 0 };
    static const int dy[] = { 0, 0, 1, -1 };

    // Random-walk outward from tiles already in the cluster
    while (static_cast<int>(cluster.size()) < clusterTarget && placed < target) {
        bool extended = false;
        for (int t = 0; t < cfg.growTries; t++) {
            auto [cx, cy] = cluster[RandInt(0, static_cast<int>(cluster.size()) - 1)];
            int d = RandInt(0, 3);
            int nx = cx + dx[d];
            int ny = cy + dy[d];

            if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) continue;
            if (map.Get(nx, ny).m_tileId != cfg.groundId) continue;

            if (TryPlaceRock(map, tileFactory, nx, ny, cfg)) {
                cluster.push_back({nx, ny});
                placed++;
                extended = true;
                break;
            }
            // rock rejected (would trap a nest) — leave it ground, try another neighbor
        }
        if (!extended) break; // cluster boxed in; let the next seed start elsewhere
    }
}

bool MapGenerator::TryPlaceRock(Map& map, const TileFactory& tileFactory,
                                 int x, int y, const MapGenCfg& cfg){
    map.ApplyTileDef(x, y, tileFactory, cfg.obstacleId);

    map.BuildPathMesh();
    if (map.ValidatePathMesh()) return true;

    // Reverting: this rock would cut a nest off from the core
    map.ApplyTileDef(x, y, tileFactory, cfg.groundId);
    return false;
}

int MapGenerator::RandInt(int lo, int hi){
    return std::uniform_int_distribution<int>(lo, hi)(m_rng);
}
