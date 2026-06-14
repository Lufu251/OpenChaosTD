#pragma once

#include <states/game_state.hpp>
#include <hud/button_list.hpp>

class MenuState : public GameState {
public:
    void OnEnter(Game& game) override;
    void OnExit(Game& game) override;

    void ProcessInput(Game& game, float dt) override;
    void Update(Game& game, float dt) override;
    void Draw(Game& game) override;

private:
    // Resume the saved game: re-activate the pack it was saved with, then load it.
    void HandleContinue(Game& game);

    bool m_hasSave = false; // whether saves/savegame.json exists (Continue enabled)

    Hud::ButtonList m_buttons;
    enum Btn { Play, Continue, Settings, Editor, Exit }; // indices into m_buttons (Add order)
};