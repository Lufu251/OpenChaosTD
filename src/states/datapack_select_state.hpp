#pragma once

#include <states/game_state.hpp>
#include <engine/systems/ui_widgets.hpp>
#include <raylib.h>

// Scrollable datapack picker reached from the main menu. Lists every installed
// pack (icon, name, author, version, description); selecting one activates it and
// transitions to the screen named by the launch intent.
class DatapackSelectState : public GameState {
public:
    // Where to go once a pack is chosen: a new game, resume a save, or the
    // editor selection screen.
    enum class Intent { Play, Continue, Edit };

    explicit DatapackSelectState(Intent intent) : m_intent(intent) {}

    void OnEnter(Game& game) override;
    void OnExit(Game& game) override;

    void ProcessInput(Game& game, float dt) override;
    void Update(Game& game, float dt) override;
    void Draw(Game& game) override;

private:
    // Selecting a pack: activate it and route to the intent's destination state.
    void SelectPack(Game& game, int index);

    Intent m_intent;
    ScrollableList m_list; // scroll/hover state + card geometry (default layout)
    Button m_backButton;
};
