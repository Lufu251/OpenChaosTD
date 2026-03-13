#include <states/menu_state.hpp>
#include <game.hpp>
#include <raylib.h>
#include <states/play_state.hpp>
#include <iostream>

void MenuState::OnEnter(Game& /*game*/) {

}

void MenuState::OnExit(Game& /*game*/) {

}

void MenuState::ProcessInput(Game& game) {
    if(game.GetInput().IsMouseRightPressed() && !game.GetInput().IsMouseConsumed()){
        std::cout << "Mouse Right Pressed First" << std::endl;
        game.GetInput().ConsumeMouseClick();
    }

    if(game.GetInput().IsMouseRightPressed() && !game.GetInput().IsMouseConsumed()){
        std::cout << "Mouse Right Pressed Second" << std::endl;
    }

    if (game.GetInput().IsPressed("Confirm"))
        game.ChangeState(std::make_unique<PlayingState>());
}

void MenuState::Update(Game& /*game*/, float dt) {

}

void MenuState::Draw(Game& game){
    const int cx = game.GetRenderer().GetGameWidth()  / 2;
    const int cy = game.GetRenderer().GetGameHeight() / 2;

    ClearBackground(DARKGRAY);
    DrawText("OPEN CHAOS TD",  cx - 180, cy - 80, 40, RAYWHITE);

    DrawText("Press ENTER to Play", cx - 140, cy, 24, LIGHTGRAY);

    DrawTexture(game.GetAssets().GetTexture("tower_freezer"), 20, 20, WHITE);
}