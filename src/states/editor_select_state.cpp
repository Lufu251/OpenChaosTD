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

    // Vertical button stack (Add order must match the Btn enum).
    m_buttons = Hud::ButtonList{};
    m_buttons.Add("PARTICLE EDITOR");
    m_buttons.Add("MAP EDITOR");
    m_buttons.Add("BACK");
    m_buttons.LayoutVertical(static_cast<float>(cx - 80), static_cast<float>(cy - 20), 160.0f, 44.0f, 54.0f);
}

void EditorSelectState::OnExit(Game& /*game*/) {}

void EditorSelectState::ProcessInput(Game& game, float /*dt*/) {
    Vector2 mouse = game.GetInput().GetMousePosition();
    bool pressed = game.GetInput().IsMousePressed(MOUSE_LEFT_BUTTON);

    bool clicked = false;
    m_buttons.Update(mouse, pressed, clicked);
    (void)clicked; // sound is played per routed action below, matching the original behavior

    if (m_buttons.Consume(Particle)) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<ParticleEditorState>());
        return;
    }
    if (m_buttons.Consume(MapEd)) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<MapEditorState>());
        return;
    }
    if (m_buttons.Consume(Back)) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<DatapackSelectState>(DatapackSelectState::Intent::Edit));
        return;
    }
    if (game.GetInput().IsPressed("Cancel")) // Cancel mirrors Back, without the click sound
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

    m_buttons.Draw(btnFont, Hud::kTextPrimary);
}
