#pragma once

#include <states/game_state.hpp>
#include <engine/systems/ui_widgets.hpp>
#include <vector>

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
    std::vector<Button> m_buttons;
    WidgetGroup m_btnGroup;
    std::vector<bool> m_raised; // one-shot click signals, one per button
    enum Btn { Particle, MapEd, Back, Count };
};
