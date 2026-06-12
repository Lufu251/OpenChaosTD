#pragma once

#include <engine/lib/grid2d.hpp>
#include <raylib.h>
#include <world/tile.hpp>
#include <world/pathfinding.hpp>

class Map{
public:
    Map() = default;

    // Accessors
    Tile& Get(int cols, int rows) {return m_grid.Get(cols, rows); }
    const Tile& Get(int cols, int rows) const {return m_grid.Get(cols, rows); }

    int GetCols() const { return m_grid.GetWidth(); }
    int GetRows() const { return m_grid.GetHeight(); }
    int GetTileSize() const { return m_tileSize; }
    const std::pair<int, int>& GetCore() const { return m_core; }
    const std::vector<std::pair<int, int>>& GetNests() const { return m_nests; }
    const std::vector<std::vector<Vector2>>& GetPaths() const { return m_paths; }
    const Grid2D<Node>& GetPathMesh() const {return m_pathMesh;}
    const Grid2D<Tile>& GetGrid() const { return m_grid; }

    // Coordinate conversion
    Vector2 TileToWorld(int x, int y) const;
    bool WorldToTile(Vector2 worldPos, int& outX, int& outY) const;

    void Create(int cols, int rows);
    void SetCore(int cols, int rows);
    void AddNest(int cols, int rows);
    void SetBuff(int cols, int rows, std::string statKey, float value, bool mul);

    // Editor support: the map editor paints tile types freely, so m_core/m_nests can
    // drift from the grid (e.g. a nest tile painted over with grass). ClearNests drops
    // the cached spawn list; RebuildGeometryFromGrid re-derives m_core/m_nests purely
    // from the painted tile types, then rebuilds the path mesh so validation matches
    // exactly what the grid shows.
    void ClearNests();
    void RebuildGeometryFromGrid();

    // Install a fully-formed grid/geometry from a save and regenerate derived data.
    // The grid already encodes tile types, modifiers and walkability (incl. tiles under
    // towers), so paths are rebuilt to match exactly. m_pathMesh/m_paths are never persisted.
    void RestoreFromSave(Grid2D<Tile> grid, int tileSize,
                         std::pair<int, int> core,
                         std::vector<std::pair<int, int>> nests);

    // Pathfinding
    void BuildPathMesh();
    bool ValidatePathMesh();
    void ConstructPaths();

private:
    Grid2D<Node> m_pathMesh;

    Grid2D<Tile> m_grid;
    int m_tileSize = 32;

    std::vector<std::pair<int, int>> m_nests; // Where enemys spawn
    std::pair<int, int> m_core; // Where enemys are going
    std::vector<std::vector<Vector2>> m_paths; // Paths from nests to core

};