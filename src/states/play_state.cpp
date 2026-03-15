#include <states/play_state.hpp>
#include <game.hpp>
#include <raylib.h>

void PlayingState::OnEnter(Game& game) {
    game.GetGameData().map.Generate(16, 16, 32);
}

void PlayingState::OnExit(Game& /*game*/) {

}

void PlayingState::ProcessInput(Game& game) {
    
}

void PlayingState::Update(Game& game, float dt) {
    
}

void PlayingState::Draw(Game& game) {
    ClearBackground(DARKGRAY);
    m_renderSystem.DrawMap(game.GetGameData().map);

    DrawText("PLAYING - map renders here", 20, 20, 20, GREEN);
    DrawText(
        TextFormat("Lives: %d   Gold: %d   Score: %d",
                   game.GetGameData().lives, game.GetGameData().gold, game.GetGameData().score),
        20, game.GetRenderer().GetGameHeight() - 30, 18, RAYWHITE
    );
}