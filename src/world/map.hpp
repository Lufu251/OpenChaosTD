#pragma once

#include <lib/grid2d.hpp>
#include <raylib.h>
#include <world/tile.hpp>
#include <systems/pathfinder.hpp>

class Map{
public:
    Map() = default;

    // Accessors
    Tile& Get(int cols, int rows) {return m_grid.Get(cols, rows); }
    const Tile& Get(int cols, int rows) const {return m_grid.Get(cols, rows); }

    int GetCols() const { return m_grid.GetWidth(); }
    int GetRows() const { return m_grid.GetHeight(); }
    int GetTileSize() const { return m_tileSize; }
    const std::pair<int, int> GetCore() const { return m_core; }
    const std::vector<std::pair<int, int>> GetNests() const { return m_nests; }
    const Grid2D<Node>& GetPathMesh() const {return m_pathfinder.mesh;}

    // Coordinate conversion
    Vector2 TileToWorld(int x, int y) const;
    bool WorldToTile(Vector2 worldPos, int& outX, int& outY) const;

    void Create(int cols, int rows);
    void SetCore(int cols, int rows);
    void AddNest(int cols, int rows);

    // Pathfinding
    void BuildFlowField();
    bool ValidatePaths();

private:
    Bfs m_pathfinder;

    Grid2D<Tile> m_grid;
    int m_tileSize = 32;

    std::vector<std::pair<int, int>> m_nests; // Where enemys spawn
    std::pair<int, int> m_core; // Where enemys are going

    int GridToIndex(int x, int y){  return y * m_grid.GetWidth() + x;}
};