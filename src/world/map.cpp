#include <world/map.hpp>
#include <content/tile_factory.hpp>

#include <systems/pathfinder.hpp>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

// --- Map methods -------------------------------------------------------------

Vector2 Map::TileToWorld(int x, int y) const {
    return {
        static_cast<float>(x * m_tileSize),
        static_cast<float>(y * m_tileSize)
    };
}

bool Map::WorldToTile(Vector2 worldPos, int& outX, int& outY) const {
    outX = static_cast<int>(std::floor(worldPos.x / m_tileSize));
    outY = static_cast<int>(std::floor(worldPos.y / m_tileSize));
    return m_grid.InBounds(outX, outY);
}

void Map::Create(int cols, int rows, const TileFactory& factory, const std::string& groundId) {
    m_grid.Resize(cols, rows);
    for (int y = 0; y < rows; y++)
        for (int x = 0; x < cols; x++)
            ApplyTileDef(x, y, factory, groundId);
}

void Map::ApplyTileDef(int cols, int rows, const TileFactory& factory, const std::string& tileId) {
    const auto& def = factory.Get(tileId);
    int texIndex = 0;
    if (!def.textures.empty())
        texIndex = GetRandomValue(0, static_cast<int>(def.textures.size()) - 1);

    Tile& tile = m_grid.Get(cols, rows);
    tile.m_tileId = tileId;
    tile.m_walkable = def.walkable;
    tile.m_buildable = def.buildable;
    tile.m_textureIndex = texIndex;
    tile.m_modifier = def.modifier;
}

void Map::SetCore(int cols, int rows) {
    m_core = {cols, rows};
}

void Map::AddNest(int cols, int rows) {
    for (auto& nest : m_nests) {
        if (nest.first == cols && nest.second == rows)
            return; // already a nest
    }

    m_nests.push_back({cols, rows});
    m_paths.push_back({}); // reserve slot for this nest's path
}

void Map::SetBuff(int cols, int rows, const TileFactory& factory, const std::string& buffId) {
    ApplyTileDef(cols, rows, factory, buffId);
}

void Map::ClearNests() {
    m_nests.clear();
    m_paths.clear();
}

void Map::RebuildGeometryFromGrid() {
    ClearNests();

    int width  = m_grid.GetWidth();
    int height = m_grid.GetHeight();

    // Re-derive the goal and spawns purely from the painted tile IDs. The first
    // goal tile wins (the editor enforces a single core when painting).
    bool coreFound = false;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const std::string& tileId = m_grid.Get(x, y).m_tileId;
            if (tileId == "core" && !coreFound) {
                m_core = {x, y};
                coreFound = true;
            } else if (tileId == "nest") {
                AddNest(x, y); // also reserves a path slot, keeping m_paths in lock-step
            }
        }
    }

    if (coreFound) {
        BuildPathMesh();
        return;
    }

    // No core to solve toward: produce a correctly-sized, all-unreachable mesh so
    // ValidatePathMesh and renderers can index it safely, and flag the missing core.
    m_pathMesh.Resize(width, height); // default Node has infinite distance
    ConstructPaths();
    m_core = {-1, -1};
}

void Map::RestoreFromSave(Grid2D<Tile> grid, int tileSize,
                          std::pair<int, int> core,
                          std::vector<std::pair<int, int>> nests) {
    m_grid     = std::move(grid);
    // Guard a non-positive tileSize from a corrupt/hand-edited save: WorldToTile divides by it, so 0
    // would be a float divide-by-zero and static_cast<int>(inf) UB. Clamp to a sane positive minimum.
    m_tileSize = std::max(1, tileSize);
    m_core     = core;
    m_nests    = std::move(nests);

    // ConstructPaths iterates m_paths and indexes m_nests in lock-step, so reserve one
    // (empty) path slot per nest — mirroring what AddNest does incrementally during setup.
    m_paths.assign(m_nests.size(), {});

    BuildPathMesh(); // regenerates m_pathMesh + m_paths from the restored walkability
}

void Map::BuildPathMesh() {
    // Adapt the tile grid into an abstract walkability mask, then let the Pathfinder solve on it.
    // The Pathfinder stays free of any Tile/Map knowledge.
    int width  = m_grid.GetWidth();
    int height = m_grid.GetHeight();

    WalkableMask walkable(width, height);
    for (int x = 0; x < width; x++)
        for (int y = 0; y < height; y++)
            walkable.Set(x, y, m_grid.Get(x, y).m_walkable ? 1 : 0);

    m_pathMesh = Pathfinder::Solve(walkable, m_core);

    ConstructPaths();
}

bool Map::ValidatePathMesh() {
    for (auto& nest : m_nests) {
        if (m_pathMesh.Get(nest.first, nest.second).m_distance == std::numeric_limits<int>::max())
            return false;
    }
    return true;
}

void Map::ConstructPaths() {
    // m_paths and m_nests are kept in lock-step (AddNest pushes both; ClearNests/RestoreFromSave
    // resize them together), so indexing m_nests[i] by the m_paths loop variable is safe. Assert the
    // invariant rather than risk a silent out-of-bounds read if the two ever drift apart.
    assert(m_paths.size() == m_nests.size() && "Map: paths/nests out of sync");

    float half = static_cast<float>(m_tileSize) / 2.0f;

    for (size_t i = 0; i < m_paths.size(); i++) {
        m_paths[i].clear();

        std::pair<int, int> nest = m_nests[i];
        if (!m_pathMesh.InBounds(nest.first, nest.second) ||
            m_pathMesh.Get(nest.first, nest.second).m_distance == std::numeric_limits<int>::max())
            continue;

        // Walk predecessor chain from nest back to core, then reverse for spawn-to-core order
        std::vector<std::pair<int, int>> nodes;
        std::pair<int, int> current = nest;
        while (m_pathMesh.Get(current.first, current.second).m_predecessor != current) {
            nodes.push_back(current);
            current = m_pathMesh.Get(current.first, current.second).m_predecessor;
        }
        nodes.push_back(current); // core itself
        std::reverse(nodes.begin(), nodes.end());

        for (auto& node : nodes) {
            m_paths[i].push_back({
                node.first  * static_cast<float>(m_tileSize) + half,
                node.second * static_cast<float>(m_tileSize) + half
            });
        }
    }
}
