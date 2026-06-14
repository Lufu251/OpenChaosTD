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

    // Rebuild the vertical button stack (index order must match the Btn enum).
    m_buttons.resize(Count);
    m_raised.resize(Count);
    m_buttons[Play].m_label     = "PLAY";
    m_buttons[Continue].m_label = "CONTINUE";
    m_buttons[Settings].m_label = "SETTINGS";
    m_buttons[Editor].m_label   = "EDITOR";
    m_buttons[Exit].m_label     = "EXIT";

    m_btnGroup.SetCount(Count);
    m_btnGroup.m_config.m_mode = WidgetGroupConfig::Mode::Vertical;
    m_btnGroup.m_config.m_pack = WidgetGroupConfig::Pack::Start;
    m_btnGroup.m_config.m_align = WidgetGroupConfig::Align::Start;
    m_btnGroup.m_config.m_bounds = {
        static_cast<float>(cx - 80),
        static_cast<float>(cy + 20),
        160.0f,
        54.0f * static_cast<float>(Count - 1) + 44.0f
    };
    m_btnGroup.m_config.m_defaultItemW = 160.0f;
    m_btnGroup.m_config.m_defaultItemH = 44.0f;
    m_btnGroup.m_config.m_gapY = 10.0f;
    m_btnGroup.Layout();
    for (int i = 0; i < Count; i++)
        m_buttons[i].m_rect = m_btnGroup[i].m_rect;
}

void MenuState::OnExit(Game& /*game*/) {

}

void MenuState::ProcessInput(Game& game, float /*dt*/) {
    Vector2 mouse = game.GetInput().GetMousePosition();
    bool pressed = game.GetInput().IsMousePressed(MOUSE_LEFT_BUTTON);

    // Continue is interactive only when a save exists.
    m_buttons[Continue].m_enabled = m_hasSave;

    // Clear previous frame's one-shot signals.
    for (int i = 0; i < Count; i++)
        m_raised[i] = false;

    for (auto& btn : m_buttons)
        btn.Update(mouse, pressed);

    // Raise one-shot signals for clicked enabled buttons.
    bool clicked = false;
    for (int i = 0; i < Count; i++) {
        if (m_buttons[i].m_enabled && m_buttons[i].IsClicked()) {
            m_raised[i] = true;
            clicked = true;
        }
    }
    (void)clicked; // sound is played per routed action below, matching the original behavior

    // Continue resumes the last save.
    if (m_raised[Continue]) {
        game.GetSoundSystem().PlaySfx("button_click");
        HandleContinue(game);
        return;
    }
    // Play and the editors both need an active datapack, so they route through the
    // selection screen first (carrying where to go once a pack is chosen).
    if (m_raised[Play] || game.GetInput().IsPressed("Confirm")) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<DatapackSelectState>(DatapackSelectState::Intent::Play));
        return;
    }
    if (m_raised[Settings]) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.PushState(std::make_unique<SettingsState>()); // Back pops to this live menu
        return;
    }
    if (m_raised[Editor]) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<DatapackSelectState>(DatapackSelectState::Intent::Edit));
        return;
    }
    if (m_raised[Exit] || game.GetInput().IsPressed("Cancel"))
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

    // The disabled Continue draws grayed; all other buttons use the primary text color.
    for (int i = 0; i < Count; i++) {
        m_buttons[i].m_fontSize = btnFont;
        m_buttons[i].m_labelColor = m_buttons[i].m_enabled ? Hud::kTextPrimary : Hud::kTextDisabled;
        m_buttons[i].Draw();
    }
}
