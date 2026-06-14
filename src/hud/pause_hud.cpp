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

    // Set up button labels (order must match the kResume..kMainMenu indices).
    m_pauseButtons.resize(kCount);
    m_raised.resize(kCount);
    m_pauseButtons[kResume].m_label   = "RESUME";
    m_pauseButtons[kSettings].m_label = "SETTINGS";
    m_pauseButtons[kSave].m_label     = "SAVE";
    m_pauseButtons[kLoad].m_label     = "LOAD";
    m_pauseButtons[kRestart].m_label  = "RESTART";
    m_pauseButtons[kMainMenu].m_label = "MAIN MENU";

    m_pauseGroup.SetCount(kCount);
    m_pauseGroup.m_config.m_mode = WidgetGroupConfig::Mode::Vertical;
    m_pauseGroup.m_config.m_pack = WidgetGroupConfig::Pack::Start;
    m_pauseGroup.m_config.m_align = WidgetGroupConfig::Align::Start;
    m_pauseGroup.m_config.m_bounds = { btnX, firstY, btnW, spacing * static_cast<float>(kCount - 1) + btnH };
    m_pauseGroup.m_config.m_defaultItemW = btnW;
    m_pauseGroup.m_config.m_defaultItemH = btnH;
    m_pauseGroup.m_config.m_gapY = spacing - btnH;
    m_pauseGroup.Layout();
    for (int i = 0; i < kCount; i++)
        m_pauseButtons[i].m_rect = m_pauseGroup[i].m_rect;
}

void PauseHUD::ProcessInput(Input& input) {
    // BeginInput also swallows clicks on the panel so they never reach the game grid behind it.
    Vector2 mousePos{};
    bool pressed = false;
    if (!BeginInput(input, mousePos, pressed)) return;

    // Clear previous frame's one-shot signals.
    for (int i = 0; i < kCount; i++)
        m_raised[i] = false;

    for (auto& btn : m_pauseButtons)
        btn.Update(mousePos, pressed);

    // Raise one-shot signals for clicked enabled buttons.
    bool clicked = false;
    for (int i = 0; i < kCount; i++) {
        if (m_pauseButtons[i].m_enabled && m_pauseButtons[i].IsClicked()) {
            m_raised[i] = true;
            clicked = true;
        }
    }
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

    for (auto& btn : m_pauseButtons) {
        btn.m_fontSize = m_metrics.fontLabel;
        btn.m_labelColor = Hud::kTextPrimary;
        btn.Draw();
    }
}
