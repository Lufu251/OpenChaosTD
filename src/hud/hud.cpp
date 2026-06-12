#include <hud/hud.hpp>
#include <hud/hud_theme.hpp>
#include <engine/core/text_renderer.hpp>
#include <engine/core/input.hpp>
#include <engine/systems/sound_system.hpp>
#include <algorithm>

void HUD::PlayClickSound() const {
    if (m_soundSystem)
        m_soundSystem->PlaySfx("button_click");
}

void DrawTextCenteredX(const char* text, int centerX, int y, int fontSize, Color color, Text::Kind kind) {
    // Thin integer-coord forwarder to the shared float-coord helper.
    DrawCenteredText(text, static_cast<float>(centerX), static_cast<float>(y), fontSize, color, kind);
}

void HUD::DrawPanelBackground(unsigned char alpha, bool border) const {
    DrawRectangleRec(m_panelRect, Hud::PanelBg(alpha));
    if (border)
        DrawRectangleLinesEx(m_panelRect, 1.0f, Hud::kPanelBorder);
}

void HUD::DrawWindowBackground() const {
    DrawPanelBackground(Hud::kPanelAlphaWindow, true);
}

void HUD::DrawDockedBackground() const {
    DrawPanelBackground(Hud::kPanelAlphaDocked, false);
}

void HUD::ConsumePanelClick(Input& input) const {
    if (input.IsMousePressed(MOUSE_LEFT_BUTTON) &&
        CheckCollisionPointRec(input.GetMousePosition(), m_panelRect))
        input.ConsumeMouseInput();
}

void HUD::ClampPanelToScreen(int screenW, int screenH) {
    m_panelRect.x = std::clamp(m_panelRect.x, 0.0f,
                               static_cast<float>(screenW) - m_panelRect.width);
    m_panelRect.y = std::clamp(m_panelRect.y, 0.0f,
                               static_cast<float>(screenH) - m_panelRect.height);
}

bool HUD::BeginInput(Input& input, Vector2& mousePos, bool& pressed) {
    if (!m_visible) return false;
    mousePos = input.GetMousePosition();
    pressed = input.IsMousePressed(MOUSE_LEFT_BUTTON);
    ConsumePanelClick(input);
    return true;
}

bool HUD::BeginInput(Input& input) {
    if (!m_visible) return false;
    ConsumePanelClick(input);
    return true;
}

// ============================================================================
// Stateless draw helpers (Hud namespace)
// ============================================================================

namespace Hud {

void DrawFramedBox(Rectangle rect, Color fill, Color border) {
    DrawRectangleRec(rect, fill);
    DrawRectangleLinesEx(rect, 1.0f, border);
}

void DrawTextRightAligned(const char* text, float rightEdge, float y, int fontSize, Color color,
                          Text::Kind kind) {
    int w = Text::Measure(text, fontSize, kind);
    Text::Draw(text, static_cast<int>(rightEdge) - w, static_cast<int>(y), fontSize, color, kind);
}

float DrawDescLines(const std::vector<DescLine>& lines, float x, float y, float lineH, int fontSize) {
    for (const auto& line : lines) {
        Text::Draw(line.m_text.c_str(), static_cast<int>(x), static_cast<int>(y), fontSize, line.m_color);
        y += lineH;
    }
    return y;
}

void DrawToggleableButton(const Button& btn, bool enabled, int fontSize, Color enabledColor) {
    // The widget style only swaps the backgrounds; the disabled label reads through the shared
    // text-disabled role so greyed-out text looks the same on every panel.
    btn.Draw(false, enabled ? kDefaultStyle : kDisabledStyle);
    btn.DrawLabel(fontSize, enabled ? enabledColor : kTextDisabled);
}

void DrawHighlightButton(const Button& btn, bool highlighted, int fontSize,
                         Color activeColor, Color normalColor) {
    btn.Draw(highlighted); // selected => gold border from kDefaultStyle
    btn.DrawLabel(fontSize, highlighted ? activeColor : normalColor);
}

void DrawOverlayToast(const char* text, Rectangle bg, float textX, float textY, int fontSize,
                      float fade) {
    auto faded = [fade](unsigned char a) { return static_cast<unsigned char>(a * fade); };
    DrawRectangleRec(bg, PanelBg(faded(kOverlayBgAlpha)));
    Text::Draw(text, static_cast<int>(textX), static_cast<int>(textY), fontSize,
               EventText(faded(kOverlayTextAlpha)));
}

} // namespace Hud
