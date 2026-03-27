#include <states/play_state.hpp>
#include <game.hpp>
#include <raylib.h>
#include <raymath.h>

void PlayingState::OnEnter(Game& game) {
    m_worldSystem.GenerateMap(game.GetGameData().map, 15, 19);

    // Center the map in the middle of the screen
    m_renderSystem.camera.target = {-static_cast<float>(game.GetRenderer().GetGameWidth()) / 2.f, -static_cast<float>(game.GetRenderer().GetGameHeight()) / 2.f};
    m_renderSystem.camera.offset = {-(game.GetGameData().map.GetCols() * game.GetGameData().map.GetTileSize()) / 2.f, -(game.GetGameData().map.GetRows() * game.GetGameData().map.GetTileSize()) / 2.f};
    m_renderSystem.camera.zoom = 1.0f;


    game.GetGameData().map.BuildFlowField();
}

void PlayingState::OnExit(Game& /*game*/) {

}

void PlayingState::ProcessInput(Game& game) {
    ControlCamera(m_renderSystem.camera, game);
    Tower tower;
    tower.m_position = {200, 200};

    if(game.GetInput().IsMouseLeftPressed()){
        m_worldSystem.PlaceTower(tower, game, m_renderSystem.camera);
    }
    
    
}

void PlayingState::Update(Game& game, float dt) {
    
}

void PlayingState::Draw(Game& game) {
    ClearBackground(DARKGRAY);

    BeginMode2D(m_renderSystem.camera);
        m_renderSystem.DrawMap(game.GetGameData().map, game.GetAssets());
        m_renderSystem.DrawTower(game.GetGameData().towers, game.GetAssets());
    EndMode2D();

    DrawText("PLAYING - map renders here", 20, 20, 20, GREEN);
    DrawText(
        TextFormat("Lives: %d   Gold: %d   Score: %d",
                   game.GetGameData().lives, game.GetGameData().gold, game.GetGameData().score),
        20, game.GetRenderer().GetGameHeight() - 30, 18, RAYWHITE
    );
}

void PlayingState::ControlCamera(Camera2D& camera, Game& game){
    // ------------------------------
    // Moving Camera
    // ------------------------------
    Vector2 direction{0,0};

    // Move camera with keyboard
    if(game.GetInput().IsDown("Up")) direction.y ++;
    if(game.GetInput().IsDown("Down")) direction.y --;
    if(game.GetInput().IsDown("Right")) direction.x --;
    if(game.GetInput().IsDown("Left")) direction.x ++;
    direction *= 6;

    // Move camera by draging
    if(game.GetInput().IsMouseRightDown()){
        direction = (game.GetInput().GetMousePosition() -mousePositionLast) / camera.zoom;
    }

    camera.target -= direction;

    // Update mousePositionLast
    mousePositionLast = game.GetInput().GetMousePosition();

    // ------------------------------
    // Zooming Camera
    // ------------------------------
    float wheel = game.GetInput().IsMouseWheelMoved();
    if(wheel != 0){
        Vector2 mouseScreen = game.GetInput().GetMousePosition();

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