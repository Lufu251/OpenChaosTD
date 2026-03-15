#pragma once

#include <core/grid2d.hpp>
#include <raylib.h>
#include <world/tile.hpp>

class Map{
public:
    Map() = default;

    // Accessors
    const Grid2D<Tile>& GetGrid() const { return m_grid; }

    int GetCols() const { return m_grid.GetWidth(); }
    int GetRows() const { return m_grid.GetHeight(); }
    int GetTileSize() const { return m_tileSize; }

    // Coordinate conversion
    Vector2 TileToWorld(int x, int y) const;
    bool WorldToTile(Vector2 worldPos, int& outX, int& outY) const;

    // TODO:
    // void check if placement is allowed
    // void calculate path

private:
    Grid2D<Tile> m_grid;
    int m_tileSize = 32;
};