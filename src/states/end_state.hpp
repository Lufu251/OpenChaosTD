#pragma once

#include <states/game_state.hpp>
#include <engine/systems/ui_widgets.hpp>

class EndState : public GameState {
public:
    explicit EndState(bool won);

    void OnEnter(Game& game) override;
    void OnExit(Game& game) override;
    void ProcessInput(Game& game, float dt) override;
    void Update(Game& game, float dt) override;
    void Draw(Game& game) override;

private:
    bool m_won;
    Button m_playAgainButton;
    Button m_menuButton;
};
