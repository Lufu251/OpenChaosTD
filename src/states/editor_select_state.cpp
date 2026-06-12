#include <states/editor_select_state.hpp>
#include <states/particle_editor_state.hpp>
#include <states/map_editor_state.hpp>
#include <states/datapack_select_state.hpp>
#include <engine/core/text_renderer.hpp>
#include <game.hpp>
#include <raylib.h>
#include <memory>

namespace {
    constexpr Color kBackground = {30, 30, 35, 255};
}

void EditorSelectState::OnEnter(Game& game) {
    int cx = game.GetScreen().GetGameWidth()  / 2;
    int cy = game.GetScreen().GetGameHeight() / 2;

    m_particleEditorButton.m_label = "PARTICLE EDITOR";
    m_particleEditorButton.m_rect = { static_cast<float>(cx - 80), static_cast<float>(cy - 20), 160.0f, 44.0f };

    m_mapEditorButton.m_label = "MAP EDITOR";
    m_mapEditorButton.m_rect = { static_cast<float>(cx - 80), static_cast<float>(cy + 34), 160.0f, 44.0f };

    m_backButton.m_label = "BACK";
    m_backButton.m_rect = { static_cast<float>(cx - 80), static_cast<float>(cy + 88), 160.0f, 44.0f };
}

void EditorSelectState::OnExit(Game& /*game*/) {}

void EditorSelectState::ProcessInput(Game& game, float /*dt*/) {
    Vector2 mouse = game.GetInput().GetMousePosition();
    bool clicked = game.GetInput().IsMousePressed(MOUSE_LEFT_BUTTON);

    m_particleEditorButton.Update(mouse, clicked);
    m_mapEditorButton.Update(mouse, clicked);
    m_backButton.Update(mouse, clicked);

    if (m_particleEditorButton.IsClicked()) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<ParticleEditorState>());
    }

    if (m_mapEditorButton.IsClicked()) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<MapEditorState>());
    }

    if (m_backButton.IsClicked() || game.GetInput().IsPressed("Cancel")) {
        if (m_backButton.IsClicked()) game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<DatapackSelectState>(DatapackSelectState::Intent::Edit));
    }
}

void EditorSelectState::Update(Game& /*game*/, float /*dt*/) {}

void EditorSelectState::Draw(Game& game) {
    const int cx = game.GetScreen().GetGameWidth()  / 2;
    const int cy = game.GetScreen().GetGameHeight() / 2;

    ClearBackground(kBackground);
    Text::Draw("SELECT EDITOR", cx - Text::Measure("SELECT EDITOR", 40) / 2, cy - 100, 40, RAYWHITE);

    m_particleEditorButton.Draw();
    m_particleEditorButton.DrawLabel(16, RAYWHITE);

    m_mapEditorButton.Draw();
    m_mapEditorButton.DrawLabel(18, RAYWHITE);

    m_backButton.Draw();
    m_backButton.DrawLabel(24, RAYWHITE);
}
