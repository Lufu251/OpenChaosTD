#include <hud/pause_hud.hpp>
#include <engine/core/input.hpp>
#include <raylib.h>

void PauseHUD::Build(float scale, int screenW, int screenH) {
    HUD::Build(scale);
    Hide(); // shown only while the game is paused

    m_screenW = screenW;
    m_screenH = screenH;
    float gw = static_cast<float>(screenW);
    float gh = static_cast<float>(screenH);

    // Centered panel sized to hold the title and six stacked buttons.
    float panelW = Scaled(240.0f);
    float panelH = Scaled(460.0f);
    m_panelRect = { (gw - panelW) / 2.0f, (gh - panelH) / 2.0f, panelW, panelH };

    float btnW = Scaled(180.0f);
    float btnH = Scaled(44.0f);
    float btnX = (gw - btnW) / 2.0f;
    float spacing = Scaled(56.0f);
    float firstY = m_panelRect.y + Scaled(80.0f);

    // Order must match the kResume..kMainMenu indices.
    m_buttons.items.clear();
    m_buttons.Add("RESUME");
    m_buttons.Add("SETTINGS");
    m_buttons.Add("SAVE");
    m_buttons.Add("LOAD");
    m_buttons.Add("RESTART");
    m_buttons.Add("MAIN MENU");
    m_buttons.LayoutVertical(btnX, firstY, btnW, btnH, spacing);
}

void PauseHUD::ProcessInput(Input& input) {
    // BeginInput also swallows clicks on the panel so they never reach the game grid behind it.
    Vector2 mousePos{};
    bool pressed = false;
    if (!BeginInput(input, mousePos, pressed)) return;

    bool clicked = false;
    m_buttons.Update(mousePos, pressed, clicked);
    if (clicked) PlayClickSound();
}

void PauseHUD::Draw() {
    if (!m_visible) return;

    // Dim the whole screen so the map, towers, and enemies stay visible behind the menu.
    DrawRectangle(0, 0, m_screenW, m_screenH, {0, 0, 0, 120});

    // Panel background reuses the shared HUD style (dark fill + subtle border).
    DrawPanelBackground(230, true);

    int centerX = static_cast<int>(m_panelRect.x + m_panelRect.width / 2.0f);
    int titleY = static_cast<int>(m_panelRect.y + Scaled(28.0f));
    DrawTextCenteredX("PAUSED", centerX, titleY, ScaledInt(28.0f), RAYWHITE);

    m_buttons.Draw(ScaledInt(18.0f), RAYWHITE);
}
