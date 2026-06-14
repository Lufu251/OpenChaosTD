#pragma once

#include <states/game_state.hpp>
#include <hud/button_list.hpp>

// Intermediate screen between DatapackSelectState and an editor, reached after
// the player selects a datapack. Offers the choice between the Particle Editor
// and the Map Editor with a back button to pick a different datapack.
class EditorSelectState : public GameState {
public:
    void OnEnter(Game& game) override;
    void OnExit(Game& game) override;

    void ProcessInput(Game& game, float dt) override;
    void Update(Game& game, float dt) override;
    void Draw(Game& game) override;

private:
    Hud::ButtonList m_buttons;
    enum Btn { Particle, MapEd, Back }; // indices into m_buttons (Add order)
};
