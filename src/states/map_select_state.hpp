#pragma once

#include <states/game_state.hpp>
#include <engine/systems/ui_widgets.hpp>
#include <raylib.h>
#include <string>
#include <vector>

// Scrollable map picker shown after a datapack is chosen and before gameplay.
// Lists the active pack's custom maps (name, description, preview image from
// "maps/<name>/map.png") plus a final "Auto-Generated" option. Selecting a map
// records the choice in GameData and starts PlayingState; Back returns to the
// datapack picker. Preview textures are owned here and freed on exit.
class MapSelectState : public GameState {
public:
    void OnEnter(Game& game) override;
    void OnExit(Game& game) override;

    void ProcessInput(Game& game, float dt) override;
    void Update(Game& game, float dt) override;
    void Draw(Game& game) override;

private:
    struct MapEntry {
        std::string m_folder;       // subdir name under maps/ (empty for the auto entry)
        std::string m_name;
        std::string m_description;
        Texture2D m_preview = {};
        bool m_hasPreview = false;
        bool m_isAuto = false;      // the synthetic "Auto-Generated" choice
    };

    void RebuildList(Game& game);   // scan maps/, parse meta, load previews
    void UnloadPreviews();          // free every loaded preview texture
    void SelectEntry(Game& game, int index);

    std::vector<MapEntry> m_entries;
    ScrollableList m_list; // scroll/hover state + card geometry (default layout)
    Button m_backButton;

    static constexpr float kIconPad = 16.0f; // inset of the preview inside a card
};
