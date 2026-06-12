#include <states/end_state.hpp>
#include <engine/core/text_renderer.hpp>
#include <hud/hud_theme.hpp>
#include <states/menu_state.hpp>
#include <states/play_state.hpp>
#include <app/game.hpp>
#include <raylib.h>

EndState::EndState(bool won) : m_won(won) {}

void EndState::OnEnter(Game& game) {
    float cx = game.GetScreen().GetGameWidth()  / 2.0f;
    float cy = game.GetScreen().GetGameHeight() / 2.0f;

    m_playAgainButton.m_label = "PLAY AGAIN";
    m_playAgainButton.m_rect = { cx - 90.0f, cy + 20.0f, 180.0f, 44.0f };

    m_menuButton.m_label = "MAIN MENU";
    m_menuButton.m_rect = { cx - 90.0f, cy + 74.0f, 180.0f, 44.0f };
}

void EndState::OnExit(Game& /*game*/) {}

void EndState::ProcessInput(Game& game, float /*dt*/) {
    Vector2 mouse = game.GetInput().GetMousePosition();
    bool clicked = game.GetInput().IsMousePressed(MOUSE_LEFT_BUTTON);

    m_playAgainButton.Update(mouse, clicked);
    m_menuButton.Update(mouse, clicked);

    if (m_playAgainButton.IsClicked()) {
        game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<PlayingState>());
    }

    if (m_menuButton.IsClicked() || game.GetInput().IsPressed("Cancel")) {
        if (m_menuButton.IsClicked()) game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<MenuState>());
    }
}

void EndState::Update(Game& /*game*/, float /*dt*/) {}

void EndState::Draw(Game& game) {
    float cx = game.GetScreen().GetGameWidth()  / 2.0f;
    float cy = game.GetScreen().GetGameHeight() / 2.0f;

    const int titleSize = static_cast<int>(Hud::kFontScreenTitle);
    const int btnFont = static_cast<int>(Hud::kFontMenuButton);

    ClearBackground(Hud::kStateBackground);

    if (m_won) {
        const char* title = "VICTORY!";
        int tw = Text::Measure(title, titleSize, Text::Kind::Title);
        Text::Draw(title, static_cast<int>(cx - tw / 2.0f), static_cast<int>(cy - 80), titleSize, Hud::kVictory, Text::Kind::Title);
    } else {
        const char* title = "GAME OVER";
        int tw = Text::Measure(title, titleSize, Text::Kind::Title);
        Text::Draw(title, static_cast<int>(cx - tw / 2.0f), static_cast<int>(cy - 80), titleSize, Hud::kDefeat, Text::Kind::Title);
    }

    m_playAgainButton.Draw();
    m_playAgainButton.DrawLabel(btnFont, Hud::kStateTextPrimary);

    m_menuButton.Draw();
    m_menuButton.DrawLabel(btnFont, Hud::kStateTextPrimary);
}
