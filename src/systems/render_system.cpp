#include <systems/render_system.hpp>

void RenderSystem::DrawMap(const Map& map, AssetManager& assets){
    for (int y = 0; y < map.GetRows(); y++) {
        for (int x = 0; x < map.GetCols(); x++) {
            DrawTexture(assets.GetTexture("tile_grass"), map.TileToWorld(x, y).x, map.TileToWorld(x, y).y, WHITE);
            //DrawRectangle(map.TileToWorld(x, y).x, map.TileToWorld(x, y).y, map.GetTileSize(), map.GetTileSize(), BLUE);
        }
    }
}

void RenderSystem::DrawTowers(const std::vector<Tower>& towers, AssetManager& assets){
    for (int y = 0; y < towers.size(); y++) {

    }
}

void RenderSystem::DrawEnemys(const std::vector<Enemy>& enemies, AssetManager& assets){
    for (int y = 0; y < enemies.size(); y++) {

    }
}