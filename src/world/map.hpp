#pragma once

#include <core/grid2d.hpp>
#include <raylib.h>
#include <world/tile.hpp>

class Map{
public:
    Map() = default;

    // Accessors
    Tile& Get(int cols, int rows) {return m_grid.Get(cols, rows); }
    const Tile& Get(int cols, int rows) const {return m_grid.Get(cols, rows); }

    int GetCols() const { return m_grid.GetWidth(); }
    int GetRows() const { return m_grid.GetHeight(); }
    int GetTileSize() const { return m_tileSize; }
    std::pair<int, int> GetCore() const { return core; }
    std::vector<std::pair<int, int>> GetNests() const { return nests; }

    // Coordinate conversion
    Vector2 TileToWorld(int x, int y) const;
    bool WorldToTile(Vector2 worldPos, int& outX, int& outY) const;

    void Create(int cols, int rows);
    void SetCore(int cols, int rows);
    void AddNest(int cols, int rows);

private:
    Grid2D<Tile> m_grid;
    int m_tileSize = 32;

    std::vector<std::pair<int, int>> nests;
    std::pair<int, int> core;
};