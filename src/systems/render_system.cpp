#include <systems/render_system.hpp>

void RenderSystem::DrawMap(const Map& map, AssetManager& assets){
    for (int y = 0; y < map.GetRows(); y++) {
        for (int x = 0; x < map.GetCols(); x++) {
            switch (map.Get(x,y).m_type) {
                case TileType::Grass:
                    DrawTexture(assets.GetTexture("tile_grass"), map.TileToWorld(x, y).x, map.TileToWorld(x, y).y, WHITE);
                    break;
                case TileType::Core:
                    DrawTexture(assets.GetTexture("tile_core"), map.TileToWorld(x, y).x, map.TileToWorld(x, y).y, WHITE);
                    break;
                case TileType::Nest:
                    DrawTexture(assets.GetTexture("tile_nest"), map.TileToWorld(x, y).x, map.TileToWorld(x, y).y, WHITE);
                    break;
                case TileType::Rock:
                    DrawTexture(assets.GetTexture("tile_rock"), map.TileToWorld(x, y).x, map.TileToWorld(x, y).y, WHITE);
                    break;
            }

            //if(x == 14 && y == 0)
                DrawText(TextFormat("%i", map.GetPathMesh().Get(x, y).distance), map.TileToWorld(x, y).x, map.TileToWorld(x, y).y, 16, BLACK);
        }
    }
}

void RenderSystem::DrawTower(const std::vector<Tower>& towers, AssetManager& assets){
    for (auto& tower : towers) {
        DrawTexture(assets.GetTexture("tower_freezer"), tower.m_position.x, tower.m_position.y, WHITE);
    }
}