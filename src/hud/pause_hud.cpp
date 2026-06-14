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
    m_metrics = Hud::PanelMetrics::Make(scale);
    m_metrics.panelW = m_metrics.Scaled(Hud::g_pauseCfg.width);
    m_metrics.panelH = m_metrics.Scaled(Hud::g_pauseCfg.height);
    m_metrics.btnW   = m_metrics.Scaled(Hud::g_pauseCfg.btnW);

    // Button height follows the label font; spacing leaves one label-font gap below each button;
    // the title baseline sits one title-font down from the panel top.
    const float btnHBase = Hud::kFontButtonLabelBase * Hud::kPauseBtnHToLabel;
    m_metrics.btnH       = m_metrics.Scaled(btnHBase);
    m_metrics.btnSpacing = m_metrics.Scaled(btnHBase + Hud::kFontButtonLabelBase);
    m_titleOffset        = m_metrics.Scaled(Hud::kFontTitleBase);

    float panelW = m_metrics.panelW;
    float panelH = m_metrics.panelH;
    m_panelRect = { (gw - panelW) / 2.0f, (gh - panelH) / 2.0f, panelW, panelH };

    float btnW = m_metrics.btnW;
    float btnH = m_metrics.btnH;
    float btnX = (gw - btnW) / 2.0f;
    float spacing = m_metrics.btnSpacing;
    float firstY = m_panelRect.y + m_metrics.Scaled(Hud::kFontTitleBase * Hud::kPauseFirstBtnToTitle);

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
    DrawRectangle(0, 0, m_screenW, m_screenH, Hud::kScreenDim);

    // Panel background reuses the shared primary-window style (dark fill + subtle border).
    DrawWindowBackground();

    int centerX = static_cast<int>(m_panelRect.x + m_panelRect.width / 2.0f);
    int titleY = static_cast<int>(m_panelRect.y + m_titleOffset);
    DrawCenteredText("PAUSED", static_cast<float>(centerX), static_cast<float>(titleY), m_metrics.fontTitle, Hud::kTextHeader);

    m_buttons.Draw(m_metrics.fontLabel, Hud::kTextPrimary);
}
