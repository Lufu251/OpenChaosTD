#include <systems/render_system.hpp>

#include <raymath.h>

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
        }
    }
}

void RenderSystem::DebugDrawMap(const Map& map){
    for (int y = 0; y < map.GetRows(); y++) {
        for (int x = 0; x < map.GetCols(); x++) {
            // Draw flowfield flow direction and distance
            if(map.GetPathMesh().Get(x, y).distance != std::numeric_limits<int>::max()){
                // Distance
                DrawText(TextFormat("%i", map.GetPathMesh().Get(x, y).distance), map.TileToWorld(x, y).x + 1, map.TileToWorld(x, y).y + 1, 6, BLACK);

                // Flow direction
                std::pair<int, int> end = map.GetPathMesh().Get(x, y).predecessor;;
                DrawLine(x * 32 +16, y * 32 +16, end.first * 32 +16, end.second * 32 +16, BLACK);
            }
        }
    }
}

void RenderSystem::DrawTowers(const DenseSlotMap<Tower>& towers, AssetManager& assets){
    for (auto& tower : towers) {
        DrawTexture(assets.GetTexture("tower_freezer"), tower.m_position.x, tower.m_position.y, WHITE);
    }
}

void RenderSystem::DrawEnemies(const DenseSlotMap<Enemy>& enemies, AssetManager& assets){
    for (auto& enemy : enemies) {
        Texture2D& texture = assets.GetTexture("enemy_voidno");
        DrawTexture(texture, enemy.m_position.x - static_cast<float>(texture.width) /2, enemy.m_position.y - static_cast<float>(texture.height) /2, WHITE);
    }
}

void RenderSystem::CenterCamera(Map& map, Renderer& renderer){
    camera.target = {-static_cast<float>(renderer.GetGameWidth()) / 2.f, -static_cast<float>(renderer.GetGameHeight()) / 2.f};
    camera.offset = {-(map.GetCols() * map.GetTileSize()) / 2.f, -(map.GetRows() * map.GetTileSize()) / 2.f};
    camera.zoom = 1.0f;
}

void RenderSystem::ControlCamera(InputManager& input){
    // ------------------------------
    // Moving Camera
    // ------------------------------
    Vector2 direction{0,0};

    // Move camera with keyboard
    if(input.IsDown("Up")) direction.y ++;
    if(input.IsDown("Down")) direction.y --;
    if(input.IsDown("Right")) direction.x --;
    if(input.IsDown("Left")) direction.x ++;
    direction *= 6;

    // Move camera by draging
    if(input.IsMouseRightDown()){
        direction = (input.GetMousePosition() -mousePositionLast) / camera.zoom;
    }

    camera.target -= direction;

    // Update mousePositionLast
    mousePositionLast = input.GetMousePosition();

    // ------------------------------
    // Zooming Camera
    // ------------------------------
    float wheel = input.IsMouseWheelMoved();
    if(wheel != 0){
        Vector2 mouseScreen = input.GetMousePosition();

        // 1. Where in the world is the mouse RIGHT NOW?
        Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);

        // 2. Shift offset to mouse (makes mouse the anchor)
        camera.offset = mouseScreen;
        camera.target = mouseWorld;

        // 3. Apply zoom
        zoomIndex += wheel;
        zoomIndex = Clamp(zoomIndex, 0, static_cast<int>(sizeof(zoomLevel) / sizeof(zoomLevel[0])) -1);

        camera.zoom = zoomLevel[zoomIndex];
    }
}