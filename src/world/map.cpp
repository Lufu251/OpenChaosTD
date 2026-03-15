#include <world/map.hpp>

Vector2 Map::TileToWorld(int x, int y) const{
    return {
        static_cast<float>(x * m_tileSize),
        static_cast<float>(y * m_tileSize)
    };
}

bool Map::WorldToTile(Vector2 worldPos, int& outX, int& outY) const{
    outX = static_cast<int>(worldPos.x) / m_tileSize;
    outY = static_cast<int>(worldPos.y) / m_tileSize;
    return m_grid.InBounds(outX, outY);
}

void Map::Generate(int cols, int rows, int tileSize){
    m_tileSize = tileSize;
    m_grid.Resize(cols, rows);
}