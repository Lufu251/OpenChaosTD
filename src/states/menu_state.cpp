#include <states/menu_state.hpp>
#include <engine/core/text_renderer.hpp>
#include <hud/hud_theme.hpp>
#include <app/game_paths.hpp>
#include <app/game.hpp>
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

    // Rebuild the vertical button stack (Add order must match the Btn enum).
    m_buttons = Hud::ButtonList{};
    m_buttons.Add("PLAY");
    m_buttons.Add("CONTINUE");
    m_buttons.Add("SETTINGS");
    m_buttons.Add("EDITOR");
    m_buttons.Add("EXIT");
    m_buttons.LayoutVertical(static_cast<float>(cx - 80), static_cast<float>(cy + 20), 160.0f, 44.0f, 54.0f);
}

void MenuState::OnExit(Game& /*game*/) {

}

void MenuState::ProcessInput(Game& game, float /*dt*/) {
    Vector2 mouse = game.GetInput().GetMousePosition();
    bool pressed = game.GetInput().IsMousePressed(MOUSE_LEFT_BUTTON);

    m_buttons.SetEnabled(Continue, m_hasSave); // Continue is interactive only when a save exists

    bool clicked = false;
    m_buttons.Update(mouse, pressed, clicked);
    (void)clicked; // sound is played per routed action below, matching the original behavior

    // Continue resumes the last save.
    if (m_buttons.Consume(Continue)) {
        game.GetSoundSystem().PlaySfx("button_click");
        HandleContinue(game);
        return;
    }
    // Play and the editors both need an active datapack, so they route through the
    // selection screen first (carrying where to go once a pack is chosen).
    if (m_buttons.Consume(Play) || game.GetInput().IsPressed("Confirm")) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<DatapackSelectState>(DatapackSelectState::Intent::Play));
        return;
    }
    if (m_buttons.Consume(Settings)) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.PushState(std::make_unique<SettingsState>()); // Back pops to this live menu
        return;
    }
    if (m_buttons.Consume(Editor)) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<DatapackSelectState>(DatapackSelectState::Intent::Edit));
        return;
    }
    if (m_buttons.Consume(Exit) || game.GetInput().IsPressed("Cancel"))
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

    const int titleSize = static_cast<int>(Hud::kFontStateTitle);
    const int btnFont = static_cast<int>(Hud::kFontMenuButton);

    ClearBackground(Hud::kWorldBackground);
    Text::Draw("OPEN CHAOS TD", cx - Text::Measure("OPEN CHAOS TD", titleSize, Text::Kind::Title)/2, cy - 80, titleSize, Hud::kTextPrimary, Text::Kind::Title);

    // The list draws the disabled Continue grayed automatically (see ButtonList::Draw).
    m_buttons.Draw(btnFont, Hud::kTextPrimary);
}