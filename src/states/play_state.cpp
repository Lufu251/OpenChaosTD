#include <states/play_state.hpp>
#include <game.hpp>
#include <raylib.h>

void PlayingState::OnEnter(Game& game) {
    game.Reset(); // Fresh lives / gold / score on every new game
}

void PlayingState::OnExit(Game& /*game*/) {}

void PlayingState::ProcessInput(Game& game) {
    
}

void PlayingState::Update(Game& game, float dt) {
    
}

void PlayingState::Draw(Game& game) {

    ClearBackground(DARKGRAY);

    DrawText("PLAYING — map renders here", 20, 20, 20, GREEN);
    DrawText(
        TextFormat("Lives: %d   Gold: %d   Score: %d",
                   game.GetLives(), game.GetGold(), game.GetScore()),
        20, game.GetRenderer().GetGameWidth() - 30, 18, RAYWHITE
    );
}