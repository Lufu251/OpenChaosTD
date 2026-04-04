#include <states/play_state.hpp>
#include <game.hpp>
#include <raylib.h>


void PlayingState::OnEnter(Game& game) {
    m_worldSystem.GenerateMap(game.GetGameData().map, 15, 19);

    // Center the map in the middle of the screen
    m_renderSystem.CenterCamera(game.GetGameData().map, game.GetRenderer());

    game.GetGameData().map.BuildFlowField();

    Enemy enemy;
    enemy.m_position = {static_cast<float>(game.GetGameData().map.GetNests()[0].first), static_cast<float>(game.GetGameData().map.GetNests()[0].second)};
    enemy.m_speed = 2;
    game.GetGameData().enemies.Insert(enemy);
}

void PlayingState::OnExit(Game& /*game*/) {

}

void PlayingState::ProcessInput(Game& game) {
    if(game.GetInput().IsPressed("Debug")) m_debug = !m_debug;

    m_renderSystem.ControlCamera(game.GetInput());

    int x, y;
    // Place tower
    if(game.GetInput().IsMouseLeftPressed() && game.GetGameData().map.WorldToTile(GetScreenToWorld2D(game.GetInput().GetMousePosition(), m_renderSystem.GetCamera()), x, y)){
        Tower tower;
        tower.m_position = {200, 200};
        m_worldSystem.PlaceTower(x, y, tower, game.GetGameData());
    }

    // Remove tower
    if(game.GetInput().IsMouseRightPressed() && game.GetGameData().map.WorldToTile(GetScreenToWorld2D(game.GetInput().GetMousePosition(), m_renderSystem.GetCamera()), x, y)){
         m_worldSystem.RemoveTower(x, y, game.GetGameData());
    }
}

void PlayingState::Update(Game& game, float dt) {
    m_worldSystem.UpdateEnemyMovement(game.GetGameData());
}

void PlayingState::Draw(Game& game) {
    ClearBackground(DARKGRAY);

    BeginMode2D(m_renderSystem.GetCamera());
        m_renderSystem.DrawMap(game.GetGameData().map, game.GetAssets());
        if(m_debug)
            m_renderSystem.DebugDrawMap(game.GetGameData().map);
        m_renderSystem.DrawTowers(game.GetGameData().towers, game.GetAssets());

        m_renderSystem.DrawEnemies(game.GetGameData().enemies, game.GetAssets());
    EndMode2D();

    DrawText("PLAYING - map renders here", 20, 20, 20, GREEN);
    DrawText(
        TextFormat("Lives: %d   Gold: %d   Score: %d",
                   game.GetGameData().lives, game.GetGameData().gold, game.GetGameData().score),
        20, game.GetRenderer().GetGameHeight() - 30, 18, RAYWHITE
    );
}