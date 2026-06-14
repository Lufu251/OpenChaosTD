#include <hud/status_hud.hpp>
#include <hud/hud_theme.hpp>
#include <hud/hud.hpp>
#include <engine/core/text_renderer.hpp>
#include <engine/core/input.hpp>
#include <raylib.h>
#include <cstdio>

void StatusHUD::Build(float scale, int screenW) {
    HUD::Build(scale);
    m_metrics = Hud::PanelMetrics::Make(scale);
    m_metrics.panelH = m_metrics.Scaled(Hud::kStatusBarBaseHeight);
    m_metrics.btnH   = m_metrics.Scaled(Hud::kFontButtonLabelBase * Hud::kStatusBtnHToLabel);
    float panelH    = m_metrics.panelH;
    float btnH      = m_metrics.btnH;
    float btnWaveW  = m_metrics.Scaled(Hud::g_statusCfg.waveW);
    float btnAutoW  = m_metrics.Scaled(Hud::g_statusCfg.autoW);
    float btnWavesW = m_metrics.Scaled(Hud::g_statusCfg.wavesW);
    float margin    = m_metrics.Scaled(Hud::g_statusCfg.margin);
    float w = static_cast<float>(screenW);

    m_panelRect = { 0.0f, 0.0f, w, panelH };
    m_textY = static_cast<int>((panelH - m_metrics.fontHeader) / 2.0f);

    float btnY = (panelH - btnH) / 2.0f;
    m_startWaveBtn.m_label = "Start Wave";
    m_startWaveBtn.m_rect = { w - btnWaveW - margin, btnY, btnWaveW, btnH };
    m_autoBtn.m_label = "Auto";
    m_autoBtn.m_rect = { w - btnWaveW - margin - btnAutoW - margin, btnY, btnAutoW, btnH };
    m_speedBtn.m_label = "1x";
    m_speedBtn.m_rect = { w - btnWaveW - margin - (btnAutoW + margin) * 2.0f, btnY, btnAutoW, btnH };
    m_waveInfoBtn.m_label = "Waves";
    m_waveInfoBtn.m_rect = { w - btnWaveW - margin - (btnAutoW + margin) * 2.0f - btnWavesW - margin,
                             btnY, btnWavesW, btnH };
}

void StatusHUD::ProcessInput(Input& input, const StatusView& view) {
    Vector2 mousePos{};
    bool pressed = false;
    if (!BeginInput(input, mousePos, pressed)) return;

    m_autoBtn.Update(mousePos, pressed);
    m_startWaveBtn.Update(mousePos, pressed);
    m_speedBtn.Update(mousePos, pressed);
    m_waveInfoBtn.Update(mousePos, pressed);

    // Auto toggle is always clickable, even mid-wave
    if (m_autoBtn.IsClicked()) {
        PlayClickSound();
        m_autoSignal.Raise();
    }

    // Speed cycle is always clickable, even mid-wave
    if (m_speedBtn.IsClicked()) {
        PlayClickSound();
        m_speedSignal.Raise();
    }

    // Wave info panel toggle is always clickable
    if (m_waveInfoBtn.IsClicked()) {
        PlayClickSound();
        m_waveInfoSignal.Raise();
    }

    // Start wave only when no wave is running
    if (!view.m_waveActive && m_startWaveBtn.IsClicked()) {
        PlayClickSound();
        m_waveSignal.Raise();
    }
}

void StatusHUD::Draw(const StatusView& view) {
    if (!m_visible) return;

    DrawDockedBackground();

    int fontMain  = m_metrics.fontHeader;
    int fontBtn   = m_metrics.fontLabel;
    int marginX   = m_metrics.ScaledInt(6.0f);
    int gapX      = m_metrics.ScaledInt(16.0f);

    // Left cluster: lives, then gold placed after the measured lives width so a large value never
    // runs under the centered readout.
    const char* livesStr = TextFormat("Lives: %d", view.m_lives);
    Text::Draw(livesStr, marginX, m_textY, fontMain, Hud::kTextPrimary, Text::Kind::Number);
    int goldX = marginX + Text::Measure(livesStr, fontMain, Text::Kind::Number) + gapX;
    Text::Draw(TextFormat("Gold: %d", view.m_gold), goldX, m_textY, fontMain, Hud::kHighlight, Text::Kind::Number);

    // Center: wave progress and win target folded into one readout (endless => infinity).
    DrawWaveReadout(view, static_cast<int>(m_panelRect.width / 2.0f));

    // Wave info panel toggle
    m_waveInfoBtn.Draw();
    m_waveInfoBtn.DrawLabel(fontBtn, Hud::kTextPrimary);

    // Speed cycle — highlighted when faster than 1x
    m_speedBtn.m_label = TextFormat("%dx", view.m_speed);
    Hud::DrawHighlightButton(m_speedBtn, view.m_speed > 1, fontBtn, Hud::kHighlight, Hud::kTextPrimary);

    // Auto toggle — highlighted when active
    Hud::DrawHighlightButton(m_autoBtn, view.m_autoSpawn, fontBtn, Hud::kHighlight, Hud::kTextPrimary);

    // Start wave button — greyed out while a wave is running
    Hud::DrawToggleableButton(m_startWaveBtn, !view.m_waveActive, fontBtn, Hud::kTextPrimary);
}

// Hand-drawn infinity glyph proportions, relative to the readout font height.
static constexpr float kInfinityWidthRatio = 1.2f; // total glyph width = ratio * font height
static constexpr float kInfinityRingRatio  = 0.30f; // ring radius = ratio * font height

void StatusHUD::DrawWaveReadout(const StatusView& view, int centerX) {
    int font = m_metrics.fontHeader;

    // Current wave number ("--" before the first wave).
    char num[16];
    if (view.m_waveNumber == 0)
        snprintf(num, sizeof(num), "--");
    else
        snprintf(num, sizeof(num), "%d", view.m_waveNumber);

    // Numeric win target: one plain centered string.
    if (view.m_victoryWave > 0) {
        DrawCenteredText(TextFormat("Wave: %s / %d", num, view.m_victoryWave),
                         static_cast<float>(centerX), static_cast<float>(m_textY), font, Hud::kTextPrimary, Text::Kind::Number);
        return;
    }

    // Endless: the target is the infinity glyph. The default font has no U+221E, so it is drawn by
    // hand; the readout is laid out in measured segments so the whole thing stays centered.
    const char* left = TextFormat("Wave: %s / ", num);
    float glyphH = static_cast<float>(font);
    float glyphW = glyphH * kInfinityWidthRatio;
    float gap = m_metrics.Scaled(2.0f);

    int leftW = Text::Measure(left, font, Text::Kind::Number);
    float totalW = static_cast<float>(leftW) + gap + glyphW;
    float startX = static_cast<float>(centerX) - totalW / 2.0f;

    Text::Draw(left, static_cast<int>(startX), m_textY, font, Hud::kTextPrimary, Text::Kind::Number);
    float glyphX = startX + static_cast<float>(leftW) + gap;
    DrawInfinity(glyphX, static_cast<float>(m_textY) + glyphH / 2.0f, glyphH, Hud::kAccent);
}

void StatusHUD::DrawInfinity(float x, float yMid, float h, Color color) const {
    // Two ring outlines side by side form the lemniscate; spans [x, x + 4r] = [x, x + 1.2*h].
    float r = h * kInfinityRingRatio;
    float th = m_metrics.Scaled(1.5f);
    float inner = r - th;
    if (inner < 0.0f) inner = 0.0f;
    DrawRing({ x + r, yMid },        inner, r, 0.0f, 360.0f, 32, color);
    DrawRing({ x + 3.0f * r, yMid }, inner, r, 0.0f, 360.0f, 32, color);
}
