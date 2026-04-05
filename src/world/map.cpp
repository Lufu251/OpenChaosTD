#include <world/map.hpp>

#include <cmath>
#include <iostream>

Vector2 Map::TileToWorld(int x, int y) const{
    return {
        static_cast<float>(x * m_tileSize),
        static_cast<float>(y * m_tileSize)
    };
}

bool Map::WorldToTile(Vector2 worldPos, int& outX, int& outY) const{
    outX = static_cast<int>(std::floor(worldPos.x / m_tileSize));
    outY = static_cast<int>(std::floor(worldPos.y / m_tileSize));
    return m_grid.InBounds(outX, outY);
}

void Map::Create(int cols, int rows){
    m_grid.Resize(cols, rows);
}

void Map::SetCore(int cols, int rows){
    m_core = {cols, rows};

    m_grid.Get(cols, rows).m_type = TileType::Core;
    m_grid.Get(cols, rows).m_buildable = false;
    m_grid.Get(cols, rows).m_walkable = true;
    std::cout << "Core placed x: " << cols << " y: " << rows << std::endl;
}

void Map::AddNest(int cols, int rows){
    // Check if that nest already exists
    for(auto& nest : m_nests){
        if(nest.first == cols && nest.second == rows){
            std::cout << "Nest not placed x: " << cols << " y: " << rows << " is already a nest" << std::endl;
            return;
        }
    }

    m_nests.push_back({cols, rows});
    m_grid.Get(cols, rows).m_type = TileType::Nest;
    m_grid.Get(cols, rows).m_buildable = false;
    m_grid.Get(cols, rows).m_walkable = true;
    std::cout << "Nest placed x: " << cols << " y: " << rows << std::endl;
}

void Map::BuildPathMesh(){
    static const int dx4[] = { 1, -1,  0,  0 };
    static const int dy4[] = { 0,  0,  1, -1 };

    int width = m_grid.GetWidth();
    int height = m_grid.GetHeight();

    Graph graph;
    graph.Resize(width, height);

    for(int x=0;x < width; x++){
        for (int y=0;y < height; y++) {

            if (!m_grid.Get(x, y).m_walkable) continue; // blocked nodes have an empty adjacency list

            for (int d = 0; d < 4; ++d) {
                int nx = x + dx4[d];
                int ny = y + dy4[d];
                if (m_grid.InBounds(nx, ny) && m_grid.Get(nx, ny).m_walkable)
                    graph.AddEdge({x, y}, {nx, ny});
            }
        }
    }

    m_pathfinder.solve({m_core.first, m_core.second}, graph);
    std::cout << "FlowField calculated" << std::endl;
}

bool Map::ValidatePathMesh(){
    for (auto& nest : m_nests) {
        if(m_pathfinder.mesh.Get(nest.first, nest.second).distance == std::numeric_limits<int>::max()){
            std::cout << "Paths are not valid" << std::endl;
            return false;
        }
    }

    std::cout << "Paths are valid" << std::endl;
    return true;
}