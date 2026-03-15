#include <systems/render_system.hpp>

void RenderSystem::DrawMap(const Map& map){
    for (int y = 0; y < map.GetRows(); y++) {
        for (int x = 0; x < map.GetCols(); x++) {
            DrawRectangle(map.TileToWorld(x, y).x, map.TileToWorld(x, y).y, map.GetTileSize(), map.GetTileSize(), BLUE);
        }
    }
}