#include <states/editor_select_state.hpp>
#include <states/particle_editor_state.hpp>
#include <states/map_editor_state.hpp>
#include <states/datapack_select_state.hpp>
#include <engine/core/text_renderer.hpp>
#include <hud/hud_theme.hpp>
#include <app/game.hpp>
#include <raylib.h>
#include <memory>

void EditorSelectState::OnEnter(Game& game) {
    int cx = game.GetScreen().GetGameWidth()  / 2;
    int cy = game.GetScreen().GetGameHeight() / 2;

    m_buttons.resize(Count);
    m_raised.resize(Count);
    m_buttons[Particle].m_label = "PARTICLE EDITOR";
    m_buttons[MapEd].m_label    = "MAP EDITOR";
    m_buttons[Back].m_label     = "BACK";

    m_btnGroup.SetCount(Count);
    m_btnGroup.m_config.m_mode = WidgetGroupConfig::Mode::Vertical;
    m_btnGroup.m_config.m_pack = WidgetGroupConfig::Pack::Start;
    m_btnGroup.m_config.m_align = WidgetGroupConfig::Align::Start;
    m_btnGroup.m_config.m_bounds = {
        static_cast<float>(cx - 80),
        static_cast<float>(cy - 20),
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

void EditorSelectState::OnExit(Game& /*game*/) {}

void EditorSelectState::ProcessInput(Game& game, float /*dt*/) {
    Vector2 mouse = game.GetInput().GetMousePosition();
    bool pressed = game.GetInput().IsMousePressed(MOUSE_LEFT_BUTTON);

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

    if (m_raised[Particle]) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<ParticleEditorState>());
        return;
    }
    if (m_raised[MapEd]) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<MapEditorState>());
        return;
    }
    if (m_raised[Back]) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<DatapackSelectState>(DatapackSelectState::Intent::Edit));
        return;
    }
    if (game.GetInput().IsPressed("Cancel"))
        game.ChangeState(std::make_unique<DatapackSelectState>(DatapackSelectState::Intent::Edit));
}

void EditorSelectState::Update(Game& /*game*/, float /*dt*/) {}

void EditorSelectState::Draw(Game& game) {
    const int cx = game.GetScreen().GetGameWidth()  / 2;
    const int cy = game.GetScreen().GetGameHeight() / 2;

    const int titleSize = static_cast<int>(Hud::kFontStateTitle);
    const int btnFont = static_cast<int>(Hud::kFontMenuButton);

    ClearBackground(Hud::kWorldBackground);
    Text::Draw("SELECT EDITOR", cx - Text::Measure("SELECT EDITOR", titleSize) / 2, cy - 100, titleSize, Hud::kTextPrimary);

    for (auto& btn : m_buttons) {
        btn.m_fontSize = btnFont;
        btn.m_labelColor = Hud::kTextPrimary;
        btn.Draw();
    }
}
