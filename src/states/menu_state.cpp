#include <states/menu_state.hpp>
#include <engine/core/text.hpp>
#include <world/game_paths.hpp>
#include <game.hpp>
#include <raylib.h>
#include <memory>
#include <string>
#include <states/datapack_select_state.hpp>
#include <states/play_state.hpp>
#include <states/settings_state.hpp>

void MenuState::OnEnter(Game& game) {
    // Returning to the menu is the single choke point that frees any active pack's
    // assets, templates and mounted resource path (idempotent if none is active).
    game.DeactivateDatapack();

    // Continue is only offered when a save exists; otherwise it shows grayed and inert.
    m_hasSave = game.GetFileStore().Exists(kSaveGamePath);

    int cx = game.GetScreen().GetGameWidth()  / 2;
    int cy = game.GetScreen().GetGameHeight() / 2;

    m_playButton.m_label = "PLAY";
    m_playButton.m_rect = { static_cast<float>(cx - 80), static_cast<float>(cy + 20), 160.0f, 44.0f };

    m_continueButton.m_label = "CONTINUE";
    m_continueButton.m_rect = { static_cast<float>(cx - 80), static_cast<float>(cy + 74), 160.0f, 44.0f };

    m_settingsButton.m_label = "SETTINGS";
    m_settingsButton.m_rect = { static_cast<float>(cx - 80), static_cast<float>(cy + 128), 160.0f, 44.0f };

    m_editorButton.m_label = "EDITOR";
    m_editorButton.m_rect = { static_cast<float>(cx - 80), static_cast<float>(cy + 182), 160.0f, 44.0f };

    m_exitButton.m_label = "EXIT";
    m_exitButton.m_rect = { static_cast<float>(cx - 80), static_cast<float>(cy + 236), 160.0f, 44.0f };
}

void MenuState::OnExit(Game& /*game*/) {

}

void MenuState::ProcessInput(Game& game, float /*dt*/) {
    Vector2 mousePos = game.GetInput().GetMousePosition();
    bool clicked = game.GetInput().IsMousePressed(MOUSE_LEFT_BUTTON);

    m_playButton.Update(mousePos, clicked);
    m_settingsButton.Update(mousePos, clicked);
    m_editorButton.Update(mousePos, clicked);
    m_exitButton.Update(mousePos, clicked);

    // Continue resumes the last save; only interactive when one exists.
    if (m_hasSave) {
        m_continueButton.Update(mousePos, clicked);
        if (m_continueButton.IsClicked()) {
            game.GetSoundSystem().PlaySfx("button_click");
            HandleContinue(game);
            return;
        }
    }

    // Play and the editors both need an active datapack, so they route through the
    // selection screen first (carrying where to go once a pack is chosen).
    if (m_playButton.IsClicked() || game.GetInput().IsPressed("Confirm")) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<DatapackSelectState>(DatapackSelectState::Intent::Play));
    }

    if (m_settingsButton.IsClicked()) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.PushState(std::make_unique<SettingsState>()); // Back pops to this live menu
    }

    if (m_editorButton.IsClicked()) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<DatapackSelectState>(DatapackSelectState::Intent::Edit));
    }

    if (m_exitButton.IsClicked() || game.GetInput().IsPressed("Cancel"))
        game.Quit();
}

void MenuState::HandleContinue(Game& game) {
    // Loading needs the save's datapack active so tower names resolve in the factory.
    nlohmann::json j = game.GetFileStore().LoadJson(kSaveGamePath); // {} if missing/corrupt
    std::string dataDir = j.value("datapack", std::string{});

    game.GetDatapackRegistry().Scan(game.GetFileStore());
    for (const auto& pack : game.GetDatapackRegistry().Packs()) {
        if (pack.DataDir() == dataDir) {
            game.ActivateDatapack(pack);
            game.ChangeState(std::make_unique<PlayingState>(true));
            return;
        }
    }

    // Pack missing/renamed (or a legacy save with no id): let the player pick one, then load.
    game.ChangeState(std::make_unique<DatapackSelectState>(DatapackSelectState::Intent::Continue));
}

void MenuState::Update(Game& /*game*/, float /*dt*/) {}

void MenuState::Draw(Game& game){
    const int cx = game.GetScreen().GetGameWidth()  / 2;
    const int cy = game.GetScreen().GetGameHeight() / 2;

    ClearBackground(DARKGRAY);
    Text::Draw("OPEN CHAOS TD", cx - Text::Measure("OPEN CHAOS TD", 40, Text::Kind::Title)/2, cy - 80, 40, RAYWHITE, Text::Kind::Title);

    // Continue is grayed and unlabeled-bright when no save is present.
    const WidgetStyle& continueStyle = m_hasSave ? kDefaultStyle : kDisabledStyle;
    m_continueButton.Draw(false, continueStyle);
    m_continueButton.DrawLabel(24, continueStyle.m_text);

    m_playButton.Draw();
    m_playButton.DrawLabel(24, RAYWHITE);

    m_settingsButton.Draw();
    m_settingsButton.DrawLabel(24, RAYWHITE);

    m_editorButton.Draw();
    m_editorButton.DrawLabel(20, RAYWHITE);

    m_exitButton.Draw();
    m_exitButton.DrawLabel(24, RAYWHITE);
}