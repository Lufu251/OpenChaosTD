#include <states/play_state.hpp>
#include <game.hpp>
#include <raylib.h>
#include <raymath.h>
#include <entities/tower.hpp>
#include <entities/enemy.hpp>

void PlayingState::OnEnter(Game& game) {
    game.GetGameData().map.Generate(16, 16, 32);

    m_renderSystem.camera.target = {-static_cast<float>(game.GetRenderer().GetGameWidth()) / 2.f, -static_cast<float>(game.GetRenderer().GetGameHeight()) / 2.f};
    m_renderSystem.camera.offset = {-(game.GetGameData().map.GetCols() * game.GetGameData().map.GetTileSize()) / 2.f, -(game.GetGameData().map.GetRows() * game.GetGameData().map.GetTileSize()) / 2.f};
    m_renderSystem.camera.rotation = 0.0f;
    m_renderSystem.camera.zoom = 1.0f;
}

void PlayingState::OnExit(Game& /*game*/) {

}

void PlayingState::ProcessInput(Game& game) {
    if(game.GetInput().IsMouseRightDown()){
        m_renderSystem.camera.offset += game.GetInput().GetMousePosition() -mousePositionLast;
    }

    mousePositionLast = game.GetInput().GetMousePosition();
}

void PlayingState::Update(Game& game, float dt) {
    
}

void PlayingState::Draw(Game& game) {
    ClearBackground(DARKGRAY);

    BeginMode2D(m_renderSystem.camera);
        m_renderSystem.DrawMap(game.GetGameData().map, game.GetAssets());
        m_renderSystem.DrawTowers(game.GetGameData().towers, game.GetAssets());
        m_renderSystem.DrawEnemys(game.GetGameData().enemies, game.GetAssets());
    EndMode2D();

    DrawText("PLAYING - map renders here", 20, 20, 20, GREEN);
    DrawText(
        TextFormat("Lives: %d   Gold: %d   Score: %d",
                   game.GetGameData().lives, game.GetGameData().gold, game.GetGameData().score),
        20, game.GetRenderer().GetGameHeight() - 30, 18, RAYWHITE
    );
}