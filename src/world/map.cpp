#include <world/map.hpp>
#include <iostream>

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

void Map::Create(int cols, int rows){
    m_grid.Resize(cols, rows);
}

void Map::SetCore(int cols, int rows){
    core = {cols, rows};

    m_grid.Get(cols, rows).m_type = TileType::Core;
    m_grid.Get(cols, rows).m_buildable = false;
    m_grid.Get(cols, rows).m_walkable = true;
    std::cout << "Core placed x: " << cols << " y: " << rows << std::endl;
}

void Map::AddNest(int cols, int rows){
    // Check if that nest already exists
    for(auto& nest : nests){
        if(nest.first == cols && nest.second == rows){
            std::cout << "Nest not placed x: " << cols << " y: " << rows << " is already a nest" << std::endl;
            return;
        }
    }

    nests.push_back({cols, rows});
    m_grid.Get(cols, rows).m_type = TileType::Nest;
    m_grid.Get(cols, rows).m_buildable = false;
    m_grid.Get(cols, rows).m_walkable = true;
    std::cout << "Nest placed x: " << cols << " y: " << rows << std::endl;
}