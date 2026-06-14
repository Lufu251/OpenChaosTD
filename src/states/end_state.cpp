#include <states/end_state.hpp>
#include <engine/core/text_renderer.hpp>
#include <hud/hud_theme.hpp>
#include <states/menu_state.hpp>
#include <states/play_state.hpp>
#include <app/game.hpp>
#include <raylib.h>

EndState::EndState(bool won) : m_won(won) {}

void EndState::OnEnter(Game& game) {
    float gw = static_cast<float>(game.GetScreen().GetGameWidth());
    float cy = game.GetScreen().GetGameHeight() / 2.0f;

    m_playAgainButton.m_label = "PLAY AGAIN";
    m_playAgainButton.m_fontSize = static_cast<int>(Hud::kFontMenuButton);
    m_playAgainButton.m_labelColor = Hud::kTextPrimary;

    m_menuButton.m_label = "MAIN MENU";
    m_menuButton.m_fontSize = static_cast<int>(Hud::kFontMenuButton);
    m_menuButton.m_labelColor = Hud::kTextPrimary;

    m_btnGroup.SetCount(2);
    m_btnGroup.m_config.m_mode = WidgetGroupConfig::Mode::Vertical;
    m_btnGroup.m_config.m_pack = WidgetGroupConfig::Pack::Start;
    m_btnGroup.m_config.m_align = WidgetGroupConfig::Align::Center;
    m_btnGroup.m_config.m_bounds = {0.0f, cy, gw, 200.0f};
    m_btnGroup.m_config.m_padTop = 20.0f;
    m_btnGroup.m_config.m_defaultItemW = 180.0f;
    m_btnGroup.m_config.m_defaultItemH = 44.0f;
    m_btnGroup.m_config.m_gapY = 10.0f;
    m_btnGroup.Layout();

    m_playAgainButton.m_rect = m_btnGroup[0].m_rect;
    m_menuButton.m_rect = m_btnGroup[1].m_rect;
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

    ClearBackground(Hud::kWorldBackground);

    if (m_won) {
        const char* title = "VICTORY!";
        int tw = Text::Measure(title, titleSize, Text::Kind::Title);
        Text::Draw(title, static_cast<int>(cx - tw / 2.0f), static_cast<int>(cy - 80), titleSize, Hud::kHighlight, Text::Kind::Title);
    } else {
        const char* title = "GAME OVER";
        int tw = Text::Measure(title, titleSize, Text::Kind::Title);
        Text::Draw(title, static_cast<int>(cx - tw / 2.0f), static_cast<int>(cy - 80), titleSize, Hud::kStatusNegative, Text::Kind::Title);
    }

    m_playAgainButton.Draw();
    m_menuButton.Draw();
}
